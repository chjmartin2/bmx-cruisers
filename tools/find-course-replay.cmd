@echo off
setlocal
cd /d "%~dp0.."
if not exist "tests\bin" mkdir "tests\bin"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
cl /nologo /std:c++17 /O2 /EHsc /W3 /I Source\BMXCruiser tools\find_course_replay.cpp Source\BMXCruiser\Core\OriginalSimulation.cpp /Fo:tests\bin\ /Fe:tests\bin\find_course_replay.exe
if errorlevel 1 exit /b %errorlevel%
tests\bin\find_course_replay.exe %*
