@ECHO OFF

call min-build-tools-only.bat
if %errorlevel% NEQ 0 goto failure
python bake.py all
if %errorlevel% NEQ 0 goto failure

goto success

:failure
echo min-build-tools-and-bake: Build failed.
exit /b 1

:success
echo min-build-tools-and-bake: Build successful.