# ATSLO RICH SYNTH

ATSLO RICH SYNTH is an original JUCE 8 hybrid instrument aimed at dark, aggressive trap production. It combines a two-oscillator polyphonic synthesizer with user-sample playback, a resonant filter, envelopes, modulation, saturation, chorus, stereo delay, reverb, stereo width, limiting, and 30 factory patches across six sound categories.

No commercial samples or recordings are included. The factory library is synthesized in real time. You can import your own WAV or AIFF file from the interface; imported audio is deliberately not embedded in plugin state, so projects fail gracefully when a user-owned source is unavailable.

## Build

Requirements are CMake 3.22+, Git, Ninja or Xcode, and Apple Clang with the macOS SDK. JUCE 8.0.6 is fetched at configure time.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --parallel 3
ctest --test-dir build --output-on-failure
```

The VST3 bundle is generated at `build/ATSLO_Rich_Synth_artefacts/Release/VST3/ATSLO RICH SYNTH.vst3`.

Install it for the current macOS user:

```sh
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
cp -R "build/ATSLO_Rich_Synth_artefacts/Release/VST3/ATSLO RICH SYNTH.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/"
```

In FL Studio, open **Options → Manage plugins**, add or select the user VST3 folder, then choose **Find installed plugins**. ATSLO RICH SYNTH should appear under installed generators. Open it in the Channel Rack and route its channel to the desired Mixer insert.

## Controls

The browser exposes 30 designed starting points. Wave chooses sine, saw, square, triangle, or noise. Blend crossfades toward a loaded sample. Detune spreads the oscillator pair. ADSR controls amplitude; Cutoff, Resonance, and Filter Env shape the resonant low-pass filter. Drive, Chorus, Delay, Reverb, Width, and Output process the final stereo signal. MIDI note velocity, two-semitone pitch bend, CC1 modulation, and CC64 sustain are accepted.

The plug-in state uses JUCE's value-tree serialization and is recalled with the DAW project. Factory patches are deterministic parameter snapshots. The Import Sample button accepts WAV and AIFF up to 60 seconds and pitch maps it from middle C.

## Validation status

The automated suite checks MIDI rendering, release behavior, parameter extremes, state restoration, factory-preset count, and WAV offline output. GitHub Actions builds and tests native ARM64 on macOS and uploads the VST3 bundle. FL Studio discovery and playback must still be validated inside an actual FL Studio installation; CI cannot establish host compatibility by itself.
