@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-windows.ps1"
if errorlevel 1 (
 echo Build failed. Install Visual Studio 2022 C++ tools, CMake and Git. See README-WINDOWS.md.
 pause
 exit /b 1
)
echo Windows VST3 is in dist\MO-VOX-Windows-x64-MSVC.zip
pause
