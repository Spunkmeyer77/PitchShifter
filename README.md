# PitchShifter

**Ticco Ross** — a three-voice harmonizer with delay-based pitch shifting.

Three pitch-shifted voices alongside your dry signal, each with its own gain, pan, semitone and cent tuning, delay and feedback. Includes five presets.

**Formats:** VST3 and standalone on macOS, Windows and Linux, plus Audio Unit on macOS.

## Download

Get the latest installers from the [Releases](../../releases) page:

| Platform | File |
|---|---|
| macOS (Apple Silicon + Intel) | `PitchShifter-x.y.z-macOS.pkg` |
| Windows (64-bit) | `PitchShifter-x.y.z-Windows-Setup.exe` or `.zip` |
| Linux (x86-64) | `PitchShifter-x.y.z-Linux.tar.gz` (run `install.sh`) |

Each release includes the user manual (`PitchShifter-Manual.pdf`).

The macOS installer isn't signed by Apple; the first time, right-click it and choose **Open**.

## Building from source

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE 8 is downloaded automatically. On Linux, install the dependencies first:

```
sudo apt install libasound2-dev libjack-jackd2-dev libfreetype6-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev
```

Every push builds all three platforms with GitHub Actions; pushing a `v*` tag publishes a release.

## License

GNU GPL v3 — see [License](License) and [NOTICE](NOTICE).
