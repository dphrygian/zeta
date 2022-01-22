@ECHO OFF

for /F %%a in ('time-get.bat') do set CURRENTTIME=%%a
set /A TIMESINCE=CURRENTTIME-%1

echo %TIMESINCE%
