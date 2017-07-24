@echo on
set
echo.

dir %BUILD_BINARIESDIRECTORY%
dir %BUILD_SOURCESDIRECTORY%
echo.

%BUILD_SOURCESDIRECTORY%\Release\CobaltFusionTest.exe 2>&1
%BUILD_SOURCESDIRECTORY%\Release\DebugView++Test.exe  2>&1

