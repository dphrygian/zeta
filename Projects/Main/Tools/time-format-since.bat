@ECHO OFF

for /F %%a in ('time-since.bat %1') do set SINCETIME=%%a
for /F %%a in ('time-format.bat %SINCETIME%') do set FORMATTIME=%%a

echo %FORMATTIME%
