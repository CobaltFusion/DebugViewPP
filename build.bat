:: if the nuget package configuration fails, make sure
:: Tools -> Options -> NuGet Package Manager -> Package Sources contains 'https://api.nuget.org/v3/index.json.'

cmake -DCMAKE_INSTALL_PREFIX=install -G "Visual Studio 17 2022" -B vs2022 .
start "" vs2022\debugviewpp.sln
