$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
cmake -S . -B build-windows-msvc -A x64 -DMOVOX_VALIDATE_HOST=ON
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
cmake --build build-windows-msvc --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Windows build failed' }
ctest --test-dir build-windows-msvc -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Windows validation failed' }
New-Item -ItemType Directory -Force dist | Out-Null
Compress-Archive -Force -Path 'build-windows-msvc/MOVOX_artefacts/Release/VST3/MO VOX.vst3', 'README-WINDOWS.md' -DestinationPath 'dist/MO-VOX-Windows-x64-MSVC.zip'
Write-Host 'Built dist/MO-VOX-Windows-x64-MSVC.zip'
