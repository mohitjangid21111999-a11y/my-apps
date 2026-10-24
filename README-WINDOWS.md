# MO VOX — Windows x64 VST3

## Installation in a Windows DAW

1. Close the DAW.
2. Extract the Windows archive. Copy the **entire `MO VOX.vst3` folder**, including its Contents folder, into `C:\Program Files\Common Files\VST3\`. Windows may request administrator permission. Do not rename the bundle or copy only its inner binary.
3. Open your 64-bit DAW and rescan VST3 plugins. Insert **MO VOX** by **MO AUDIO** on a mono or stereo vocal audio track.
4. Start at MO Default, play the vocal, and lower COMP for more compression. Raise GATE gently for noise between phrases. GAIN sets the output gain and sample-peak ceiling.

A 64-bit Windows VST3 host is required (for example REAPER, Cubase, Studio One, Ableton Live or FL Studio with VST3 enabled). These DAWs are intended targets; compatibility with each has not been individually tested. This is not a 32-bit, AAX, AU, or standalone executable. Do not run the inner plugin binary as an app. Linux VST3 bundles cannot be used on Windows.

The limiter introduces 5 ms of latency, reported to the DAW. It limits digital sample peaks, not oversampled true peaks. Bypass passes latency-matched dry audio without ceiling protection. The binary is unsigned. Windows DAW validation and vocal listening tests remain necessary before a production release. Flow Music/Flow Effect integration has not been verified.

## Build using Microsoft Visual Studio

Install Git, CMake 3.22+ and Visual Studio 2022 with **Desktop development with C++** and a Windows SDK. From PowerShell at the project root:

```powershell
./scripts/build-windows.ps1
```

This builds x64 VST3 with MSVC, runs the DSP, native processor and VST3 host-loading checks, and writes `dist/MO-VOX-Windows-x64-MSVC.zip`. JUCE 8.0.6 is fetched at a pinned commit with normal Git authentication. JUCE 8 rejects MinGW, so building this plugin with MinGW is unsupported. Use the MSVC workflow above.

The Windows GitHub Actions workflow runs the same script on `windows-2022` and uploads the zip. It has not been executed by this cloud Linux task. Commit/push the project to your repository, then run **Windows x64 VST3** under Actions to obtain that native MSVC build.

See README.md for full controls, preset handling and JUCE licensing. A suitable JUCE license is required for distribution.
