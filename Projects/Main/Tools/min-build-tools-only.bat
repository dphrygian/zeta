@ECHO OFF
REM Just build the tools, not the game

REM Updated for Karras
set VSDIR=C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE

"%VSDIR%\devenv" /build "Release|Win32" "../../../Code/Zeta_vs2015.sln" /project FontGenerator
if %errorlevel% NEQ 0 goto failure
"%VSDIR%\devenv" /build "Release|Win32" "../../../Code/Zeta_vs2015.sln" /project MeshCompiler
if %errorlevel% NEQ 0 goto failure
"%VSDIR%\devenv" /build "Release|Win32" "../../../Code/Zeta_vs2015.sln" /project ConfigCompiler
if %errorlevel% NEQ 0 goto failure
"%VSDIR%\devenv" /build "Release|Win32" "../../../Code/Zeta_vs2015.sln" /project FilePacker
if %errorlevel% NEQ 0 goto failure
"%VSDIR%\devenv" /build "Release|Win32" "../../../Code/Zeta_vs2015.sln" /project RoomBaker
if %errorlevel% NEQ 0 goto failure

REM Copy all the tools for Zeta
cp ../../../Code/Tools/FontGenerator/Win32/Release/FontGenerator.exe .
cp ../../../Code/Tools/MeshCompiler/Win32/Release/MeshCompiler.exe .
cp ../../../Code/Tools/ConfigCompiler/Win32/Release/ConfigCompiler.exe .
cp ../../../Code/Tools/FilePacker/Win32/Release/FilePacker.exe .
cp ../../../Code/Tools/RoomBaker/Win32/Release/RoomBaker.exe .

goto success

:failure
echo min-build-tools-only: Build failed.
exit /b 1

:success
echo min-build-tools-only: Build successful.