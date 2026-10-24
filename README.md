# my-apps
random apps

# MO VOX — MO AUDIO

**Windows DAW target:** see [Windows installation and build instructions](README-WINDOWS.md). Use a Windows x64 VST3 bundle; the earlier Linux binary does not run in Windows DAWs.

A native mono/stereo vocal dynamics effect with original DSP and a JUCE editor. This is working C++ audio processing, not a web mockup. Targets VST3 on Linux/macOS/Windows, and AU on macOS. No Flow Music SDK was available; a Flow Effect adapter and compatibility have **not** been implemented or verified. Use the VST3 only in hosts supporting your operating system and architecture.

## Build

Requires CMake 3.22+, C++17, Git and JUCE 8.0.6 (fetched at a fixed commit). Linux dependencies: `libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig-dev libgl1-mesa-dev`. macOS requires Xcode; Windows requires Visual Studio C++.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 2
ctest --test-dir build --output-on-failure
```

VST3 output: `build/MOVOX_artefacts/Release/VST3/MO VOX.vst3`. Copy the whole bundle to the platform's VST3 directory (`~/.vst3` on Linux). Rescan plugins in the DAW. Match the binary OS and architecture to the host. AU builds are generated only on macOS. No signing or notarization is provided.

DSP-only validation without JUCE or GUI dependencies:

```sh
cmake -S . -B build-dsp -DMOVOX_PLUGIN=OFF
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

## Operation

- GATE: Off at the bottom (-90 internal sentinel), otherwise -89.9 to 0 dBFS; soft-knee downward expansion with hold and smoothed envelope.
- COMP: 0 to -36 dB. Lower values increase ratio, lower threshold and add coordinated makeup gain. Attack and program-dependent release are internal.
- GAIN: -36 to 0 dB output gain and digital **sample-peak** ceiling. The stereo-linked lookahead limiter reports 5 ms latency to the host. This is not an oversampled true-peak limiter; reconstructed intersample peaks may exceed the ceiling.
- Meters show input peak energy, gate + compressor attenuation, and L/R output peaks. Gain reduction excludes the limiter and makeup gain. Click the panel to reset output peak holds.
- Edit the value box for numeric input; double-click a fader to reset. JUCE's modifier drag supports fine adjustments. Three themes are under Settings.
- The categorized selector and arrow buttons load all 38 original MO presets. `*` marks changes from the selected factory preset. Host automation and DAW session state are supported for all audio controls.
- A/B swaps parameter snapshots; Undo/Redo use JUCE's state undo manager. Settings provides XML user-preset save/export and load/import. Saving with an existing filename replaces that preset. User files persist under the OS application-data directory, `MO AUDIO/MO VOX/Presets`; Rename and Delete are available in Settings.
- Bypass crossfades to latency-matched dry audio. As with normal DAW bypass, the dry path is not limited. Bypass of an already loud incoming signal can be loud.

## Validation and release status

The Linux x86_64 VST3 was built and loaded through JUCE’s VST3 host. Native processing, session recall, preset values, meter atomics, and DSP checks passed. The actual editor was rendered for visual inspection. This validates a native host path, not Flow Effect or any particular commercial DAW.

For this prepared cloud instance, run `scripts/build-cloud.sh`; it uses local dependencies outside the checkout and also runs VST3 host validation.

The automated DSP suite exercises real generated audio, all 38 presets, seven sample rates from 22.05 to 384 kHz, mono/stereo, gate attenuation, compressor reduction, output gain, ceiling bounds and nonfinite input protection. It is not a listening evaluation. A production release still requires vocal listening sessions, host compatibility tests, GUI/accessibility checks, automation stress testing, macOS AU validation, and Windows/macOS builds. No proprietary Waves algorithms or assets are used, and no official Waves presets are included.

JUCE is dual licensed (AGPLv3/commercial). Obtain an appropriate JUCE license before proprietary distribution. This repository does not grant rights to JUCE or other third-party components.
