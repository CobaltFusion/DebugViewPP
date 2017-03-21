1) ./bootstrap.bat

2) look at project-config.jam, and set the path to visual studio:

import option ; 

using msvc : 14.0 : "c:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.10.24728\bin\HostX64\x64\cl.exe"; 

option.set keep-going : false ;

3) Run b2 from the visual studio command prompt

32bit: b2 toolset=msvc-14.0 address-model=32 --build-type=complete stage install
64bit: b2 toolset=msvc-14.0 address-model=64 --build-type=complete stage install

rem this output its results into c:\boost 
rem however, static libs are found in a boost-subdirectory and must be copied manually



notes:
"C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\setenv.cmd"  /Release  /x64
