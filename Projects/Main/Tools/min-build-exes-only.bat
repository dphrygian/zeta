@ECHO OFF
REM Just build the minimum projects, and don't do any extra tests

REM Updated for Karras
set VSDIR=C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE

"%VSDIR%\devenv" /build "Final|Win32" "../../../Code/Zeta_vs2015.sln" /project Rosa
if %errorlevel% NEQ 0 goto failure
"%VSDIR%\devenv" /build "Debug|x64" "../../../Code/Zeta_vs2015.sln" /project Rosa
if %errorlevel% NEQ 0 goto failure
"%VSDIR%\devenv" /build "Release|x64" "../../../Code/Zeta_vs2015.sln" /project Rosa
if %errorlevel% NEQ 0 goto failure
"%VSDIR%\devenv" /build "Final|x64" "../../../Code/Zeta_vs2015.sln" /project Rosa
if %errorlevel% NEQ 0 goto failure
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

REM Copy all the tools and executables for Zeta
cp ../../../Code/Projects/Rosa/Win32/Final/Zeta.exe ../Raw/Zeta_x86.exe
cp ../../../Code/Projects/Rosa/Win32/Final/Zeta.pdb ../Raw/Zeta_x86.pdb
cp ../../../Code/Projects/Rosa/x64/Debug/Zeta.exe ../Raw/Zeta-Debug_x64.exe
cp ../../../Code/Projects/Rosa/x64/Release/Zeta.exe ../Raw/Zeta-Release_x64.exe
cp ../../../Code/Projects/Rosa/x64/Final/Zeta.exe ../Raw/Zeta_x64.exe
cp ../../../Code/Projects/Rosa/x64/Final/Zeta.pdb ../Raw/Zeta_x64.pdb

cp ../../../Code/Tools/FontGenerator/Win32/Release/FontGenerator.exe .
cp ../../../Code/Tools/MeshCompiler/Win32/Release/MeshCompiler.exe .
cp ../../../Code/Tools/ConfigCompiler/Win32/Release/ConfigCompiler.exe .
cp ../../../Code/Tools/FilePacker/Win32/Release/FilePacker.exe .
cp ../../../Code/Tools/RoomBaker/Win32/Release/RoomBaker.exe .

goto success

:failure
echo min-build-exes-only: Build failed.
exit /b 1

:success
echo min-build-exes-only: Build successful.