./bootstrap.bat

rem this output its results into c:\boost 
rem however, static libs are found in a boost-subdirectory and must be copied manually
b2 --build-type=complete stage install

"C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\setenv.cmd"  /Release  /x64

b2 address-model=64 --build-type=complete stage install
