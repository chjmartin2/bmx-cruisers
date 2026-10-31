@echo off
setlocal
cd /d "%~dp0.."
if not exist "tests\bin" mkdir "tests\bin"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
cl /nologo /std:c++17 /O2 /EHsc /W3 /I Source\BMXCruiser tests\display_layout.cpp /Fo:tests\bin\ /Fe:tests\bin\display_layout.exe
if errorlevel 1 exit /b %errorlevel%
tests\bin\display_layout.exe
