@echo off
cd %~dp0
SETLOCAL

set BIN="C:\Program Files\Java\jdk1.7.0_40\bin"

%BIN%\javac -cp ".;*" nl/cobaltfusion/debugviewpp/*.java
%BIN%\jar cmvf Manifest.txt HelloJar.jar nl/cobaltfusion/debugviewpp/*.class jna-4.1.0.jar

ENDLOCAL
