@echo off
set file=%1
for /f "tokens=3,4,5,* delims=, " %%i in (%file%) do (
	set major=%%i
	set minor=%%j
	set inc=%%k
	set /a buildnr=%%l+1
	goto break
)
:break
echo #define VERSION %major%,%minor%,%inc%,%buildnr% > %file%
echo #define VERSION_STR "%major%.%minor%.%inc%.%buildnr%" >> %file%
