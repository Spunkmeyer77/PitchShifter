#pragma once
#include <JuceHeader.h>

#define MAX_NUM_CH 2

//#define DEFAULT_R 0.25
//#define NAME_R "release"

//#define DEFAULT_F 1000.0f
//#define DEFAULT_Q 0.707f
#define NAME_Q "quality"

//class ReleaseFilter
//{
//public:
//	ReleaseFilter() {}
//	~ReleaseFilter() {}
//
//	void prepareToPlay(double sr)
//	{
//		sampleRate = sr;
//		updateAlpha();
//	}
//
//	void processBlock(AudioBuffer<float>& buffer, const int numSamples)
//	{
//		float* bufferData = buffer.getWritePointer(0);
//
//		for (int smp = 0; smp < numSamples; ++smp)
//		{
//			envelope = jmax(bufferData[smp], envelope * (float)alpha);
//			bufferData[smp] = envelope;
//		}
//	}
//
//	void setRelease(float newValue)
//	{
//		release = newValue;
//		updateAlpha();
//	}
//
//private:
//
//	void updateAlpha()
//	{
//		const auto n = jmax(1.0, release * sampleRate);
//		alpha = exp(-1.0 / n);
//	}
//
//	float release = DEFAULT_R;
//	double sampleRate = 1.0;
//	double alpha = 0.0;
//	float envelope = 0.0f;
//};

class Filter
{
public:
	Filter()
	{
		for (int f = 0; f < MAX_NUM_CH; ++f)
			iirFilters.add(new IIRFilter());
	}

	~Filter() {}

	void prepareToPlay(double sr)
	{
		sampleRate = sr;

		reset();

		updateCoefficients();
	}

	void processBlock(AudioBuffer<float>& buffer, const int numSamples)
	{
		for (int ch = buffer.getNumChannels(); --ch >= 0;)
			iirFilters.getUnchecked(ch)->processSamples(buffer.getWritePointer(ch), numSamples);
	}

	void setFrequency(float newValue)
	{
		frequency = newValue;
		updateCoefficients();
	}

	void setQuality(float newValue)
	{
		quality = newValue;
		updateCoefficients();
	}

	void reset()
	{
		for (int f = iirFilters.size(); --f >= 0;)
			iirFilters.getUnchecked(f)->reset();
	}

private:

	void updateCoefficients()
	{
		// Esercizio: un bello switch-case col tipo di filtro scelto da parametro
		// Keep the cutoff well below Nyquist: a biquad tuned right at Nyquist is numerically
		// unstable (it produced NaNs on Apple Silicon). Also guard against an unknown sample rate.
		if (sampleRate <= 0.0)
			return;
		const double safeFrequency = jlimit(10.0, jmin(sampleRate * 0.45, 20000.0), (double) frequency);
		const IIRCoefficients iirCoeffs = IIRCoefficients::makeLowPass(sampleRate, safeFrequency, quality);
		// (fine swirtch)

		for (int f = iirFilters.size(); --f >= 0;)
			iirFilters.getUnchecked(f)->setCoefficients(iirCoeffs);
	}

	float frequency = DEFAULT_FILTER;
	float quality = DEFAULT_QFILTER;
	double sampleRate = 44100.0;

	OwnedArray<IIRFilter> iirFilters;
};