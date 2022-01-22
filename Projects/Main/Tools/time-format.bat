@ECHO OFF

set /A HH=%1/(60*60*100), REMAINDER=%1%%(60*60*100), MM=REMAINDER/(60*100), REMAINDER%%=60*100, SS=REMAINDER/100, CC=REMAINDER%%100
if %MM% lss 10 set MM=0%mm%
if %SS% lss 10 set SS=0%ss%
if %CC% lss 10 set CC=0%cc%

echo %HH%:%MM%:%SS%.%CC%
