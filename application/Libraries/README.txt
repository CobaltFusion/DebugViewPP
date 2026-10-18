These libraries are included in the archive:

PropertyGrid is written by Bjarke Viksoe.
See: http://www.viksoe.dk/code/propertygrid.htm

TabbingFramework is written by Bjarke Viksoe and Daniel Bowen.
See: http://www.codeproject.com/Articles/2431/Custom-Tab-Controls-Tabbed-Frame-and-Tabbed-MDI

nlohmann_json (3.11.3) is written by Niels Lohmann, MIT license. See: https://github.com/nlohmann/json

pugixml (1.14) is written by Arseny Kapoulkine, MIT license. See: https://pugixml.org

pcre2 (10.49) is written by Philip Hazel, BSD license. See: https://github.com/PCRE2Project/pcre2

The three are included unmodified (nlohmann_json is the single header json.hpp, pugixml is pugixml.cpp/.hpp/pugiconfig.hpp), with their license files.
pcre2 is the 8-bit library with the JIT, built as its NON-AUTOTOOLS-BUILD file describes:
config.h, pcre2.h and pcre2_chartables.c are src/config.h.generic, src/pcre2.h.generic and src/pcre2_chartables.c.dist of the release, only renamed.

To update one, replace the files with those of the new release and change the version here and in the CMakeLists.txt.
