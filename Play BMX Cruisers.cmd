@echo off
setlocal
cd /d "%~dp0"
if exist "Build\Release1.1Final\Windows\BMXCruiser.exe" (
  start "BMX Cruisers" "Build\Release1.1Final\Windows\BMXCruiser.exe"
  exit /b
)
if exist "Build\Release1.1Game\Windows\BMXCruiser.exe" (
  start "BMX Cruisers" "Build\Release1.1Game\Windows\BMXCruiser.exe"
  exit /b
)
if exist "Build\Windows\Windows\BMXCruiser.exe" (
  start "BMX Cruisers" "Build\Windows\Windows\BMXCruiser.exe"
  exit /b
)
if exist "Build\Windows\BMXCruiser.exe" (
  start "BMX Cruisers" "Build\Windows\BMXCruiser.exe"
  exit /b
)
echo BMX Cruisers has not been packaged yet.
echo Run tools\build.ps1 -Package after preparing Unreal Engine.
pause
