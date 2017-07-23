@echo on
set
echo.

dir %BUILD_BINARIESDIRECTORY%
dir %BUILD_SOURCESDIRECTORY%
echo.

%BUILD_SOURCESDIRECTORY%\Release\CobaltFusionTest.exe
%BUILD_SOURCESDIRECTORY%\Release\DebugView++Test.exe
