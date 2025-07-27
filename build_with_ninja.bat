:: building with ninja is faster (~20 seconds) compared to building with the "Visual Studio 17 2022" generator (~26 seconds)
:: however, our CI (Appveyor) works well with visual studio solution files, so we provide a way to create both.

set "NINJA_STATUS=[%%e %%f/%%t @%%r] "
cmake -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=Release -G Ninja -B build .
ninja -C build
