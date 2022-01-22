@ECHO OFF

for /F %%a in ('time-get.bat') do set STARTTIME=%%a

call min-build-exes-only.bat
if %errorlevel% NEQ 0 goto failure
python bake.py all
if %errorlevel% NEQ 0 goto failure

goto success

:failure
echo min-build-and-bake: Build failed.
exit /b 1

:success
for /F %%a in ('time-format-since.bat %STARTTIME%') do set FORMATTOTALTIME=%%a
echo min-build-and-bake: Time elapsed: %FORMATTOTALTIME%

echo min-build-and-bake: Build successful.