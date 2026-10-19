// Loads the built plugin (VST3 file or an installed AU) in a minimal host, plays a
// 440 Hz sine through it with the default settings and with every factory-style
// layout (mono and stereo), and fails if the output is silent or not finite.
//
// usage: AudioPassTest <path-to.vst3 | AU:aufx,Tpsh,Tico>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include <cmath>

using namespace juce;

static void setParam (AudioPluginInstance& p, const String& name, float plainValue)
{
    for (auto* prm : p.getParameters())
        if (prm->getName (64) == name)
            if (auto* r = dynamic_cast<RangedAudioParameter*> (prm))
                r->setValueNotifyingHost (r->convertTo0to1 (plainValue));
            else
                prm->setValueNotifyingHost (prm->getValueForText (String (plainValue)));
}

static bool runLayout (AudioPluginInstance& p, int numCh, double sr, int block)
{
    const auto set = numCh == 1 ? AudioChannelSet::mono() : AudioChannelSet::stereo();
    AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (set);
    layout.outputBuses.add (set);

    if (! p.setBusesLayout (layout))
    {
        printf ("  %d ch: layout not supported by plugin\n", numCh);
        return numCh == 1;   // mono is optional, stereo is required
    }

    p.setRateAndBufferSizeDetails (sr, block);
    p.prepareToPlay (sr, block);

    AudioBuffer<float> buffer (numCh, block);
    MidiBuffer midi;
    double phase = 0;
    float outSum = 0;
    int blocks = (int) (sr * 2.0 / block), counted = 0;
    bool finite = true;

    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < block; ++i)
        {
            const float v = 0.5f * (float) std::sin (phase);
            phase += MathConstants<double>::twoPi * 440.0 / sr;
            for (int c = 0; c < numCh; ++c)
                buffer.setSample (c, i, v);
        }

        p.processBlock (buffer, midi);

        for (int c = 0; c < numCh; ++c)
            for (int i = 0; i < block; ++i)
                finite = finite && std::isfinite (buffer.getSample (c, i));

        if (b >= blocks / 2)   // second half, after smoothing settles
        {
            for (int c = 0; c < numCh; ++c)
                outSum += buffer.getRMSLevel (c, 0, block);
            counted += numCh;
        }
    }

    p.releaseResources();
    const float outRms = outSum / (float) jmax (1, counted);
    const bool ok = finite && outRms > 0.05f && outRms < 2.0f;   // audible, and not blowing up
    printf ("  %d ch @ %.0f Hz / %d: input RMS 0.354, output RMS %.4f%s -> %s\n",
            numCh, sr, block, outRms, finite ? "" : " (NaN/Inf!)", ok ? "OK" : "FAIL");
    return ok;
}

int main (int argc, char** argv)
{
    if (argc < 2) { printf ("usage: AudioPassTest <plugin>\n"); return 2; }

    ScopedJuceInitialiser_GUI init;
    AudioPluginFormatManager formats;
    formats.addDefaultFormats();

    String target (argv[1]);
    OwnedArray<PluginDescription> found;

    for (auto* f : formats.getFormats())
    {
        if (target.startsWith ("AU:") && f->getName() == "AudioUnit")
        {
            // e.g. AU:aufx,Tpsh,Tico -> scan installed AUs and pick the matching one
            auto ids = StringArray::fromTokens (target.substring (3), ",", "");
            for (auto& id : f->searchPathsForPlugins ({}, true, false))
                if (id.containsIgnoreCase (ids[1]) && id.containsIgnoreCase (ids[2]))
                    f->findAllTypesForFile (found, id);
        }
        else if (! target.startsWith ("AU:") && f->getName() == "VST3")
        {
            f->findAllTypesForFile (found, target);
        }
    }

    if (found.isEmpty()) { printf ("FAIL: plugin not found: %s\n", argv[1]); return 1; }

    String error;
    auto plugin = formats.createPluginInstance (*found[0], 48000.0, 512, error);
    if (plugin == nullptr) { printf ("FAIL: could not load: %s\n", error.toRawUTF8()); return 1; }

    printf ("%s (%s %s)\n", plugin->getName().toRawUTF8(),
            found[0]->pluginFormatName.toRawUTF8(), found[0]->manufacturerName.toRawUTF8());

    bool ok = true;
    struct Case { int channels; double sampleRate; int blockSize; };
    const Case cases[] { { 2, 44100.0, 512 }, { 2, 48000.0, 1024 }, { 1, 48000.0, 256 } };
    for (const auto& c : cases)
        ok = runLayout (*plugin, c.channels, c.sampleRate, c.blockSize) && ok;

    // Hosts such as AU hosts often restore parameters BEFORE prepareToPlay.
    // Mute the dry voice and set the shifted voices, then check they are audible.
    printf ("  harmonies only, parameters set before prepare:\n");
    {
        String error2;
        auto fresh = formats.createPluginInstance (*found[0], 48000.0, 512, error2);
        setParam (*fresh, "Mute", 1.0f);
        setParam (*fresh, "Semitones Voice1", 4.0f);
        setParam (*fresh, "Semitones Voice2", 7.0f);
        setParam (*fresh, "Semitones Voice3", -5.0f);
        setParam (*fresh, "Cent Voice1", 0.1f);
        ok = runLayout (*fresh, 2, 48000.0, 512) && ok;
    }

    plugin.reset();
    printf (ok ? "PASS\n" : "FAIL\n");
    return ok ? 0 : 1;
}
