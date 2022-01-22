@ECHO OFF

REM Unless I need to do a full rebuild of content, this is the one-button
REM solution to making the shipping package.

for /F %%a in ('time-get.bat') do set STARTTIME=%%a

REM Update the version number before everything else so it gets into the content packages
python increment-version.py
if %errorlevel% NEQ 0 goto failure

REM Build exes and bake content to make sure we're up to date
call min-build-and-bake.bat
if %errorlevel% NEQ 0 goto failure

REM Build package files (including cloud/DLC)
python pack.py
if %errorlevel% NEQ 0 goto failure

goto success

:failure
echo build-version: Build failed.
exit /b 1

:success
for /F %%a in ('time-format-since.bat %STARTTIME%') do set FORMATTOTALTIME=%%a
echo build-version: Time elapsed: %FORMATTOTALTIME%

echo build-version: Build successful.
