@ECHO OFF

for /F "tokens=1-4 delims=:.," %%a in ("%TIME%") do (
   set /A "CURRENTTIME=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
)

echo %CURRENTTIME%
