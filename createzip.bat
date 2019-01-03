@echo off
pushd %~dp0

setlocal

if [%1] == [] (
    echo Usage: createzip ^<binary path^> ^<project path^>
    goto exit
)
if [%2] == [] (
    echo Usage: createzip ^<binary path^> ^<project path^>
    goto exit
)

set bin_dir=%1
set project_dir=%2

:: paths can be absolute or relative to the location of this batch file
set zip_bin=.\zip.exe
set upx_bin=.\upx.exe

if not exist %zip_bin% (
   echo %zip_bin% does not exist, zipfile creation skipped
   goto exit
)

if not exist %upx_bin% (
   echo %upx_bin% does not exist, exe compression skipped
   goto zip
)

:: UPX compresses executables in place about 40%, has no decompressing memory overhead and is extremely fast
%upx_bin% %bin_dir%\DebugView++.exe
%upx_bin% %bin_dir%\DebugViewConsole.exe

:: notice we pack the win32 .vsix in any case (so also in x64 builds), and this is correct.

:zip
%zip_bin% -j %bin_dir%\DebugView++.zip %bin_dir%\DebugView++.exe %bin_dir%\DebugView++*.pdb %bin_dir%\DebugViewConsole.exe %project_dir%\release\*.vsix

:exit