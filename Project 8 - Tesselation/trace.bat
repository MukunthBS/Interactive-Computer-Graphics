del .\main.trace
pause
apitrace trace --output=.\main.trace main.exe teapot_normal.png
pause
qapitrace .\main.trace
pause