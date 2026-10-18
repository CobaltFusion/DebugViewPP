@echo off

rem Every run raises the build number by one, unless the environment variable DEBUGVIEW_KEEP_BUILD_NUMBER is set (utils/release.py does that,
rem it needs the exact version that is in version.h).

echo UpdateBuildNr.cmd updating %1
echo UpdateBuildNr.cmd updating %2

set file=%1
set xmlfile=%2
for /f "tokens=3,4,5,* delims=, " %%i in (%file%) do (
	set major=%%i
	set minor=%%j
	set inc=%%k
	set /a buildnr=%%l
	if not defined DEBUGVIEW_KEEP_BUILD_NUMBER set /a buildnr=%%l+1
	goto break
)
:break
echo #define VERSION %major%,%minor%,%inc%,%buildnr% > %file%
echo #define VERSION_STR "%major%.%minor%.%inc%.%buildnr%" >> %file%
echo ^<?xml version="1.0" encoding="utf-8"?^> > %xmlfile%
echo ^<Include^> >> %xmlfile%
echo     ^<?define ProductVersion.Major="%major%" ?^> >> %xmlfile%
echo     ^<?define ProductVersion.Minor="%minor%" ?^> >> %xmlfile%
echo     ^<?define ProductVersion.Revision="%inc%" ?^> >> %xmlfile%
echo     ^<?define ProductVersion.Build="%buildnr%" ?^> >> %xmlfile%
echo     ^<?define ProductVersion="%major%.%minor%.%inc%.%buildnr%" ?^> >> %xmlfile%
echo ^</Include^> >> %xmlfile%

