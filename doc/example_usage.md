## An {fmt} v7 or earlier wrapper around OutputDebugString

This makes sending formatted message much more plesant and works on almost _any_ compiler, even with just C++11.

see: https://godbolt.org/z/ab68shhfP

Notice the example uses gcc4 to demonstrate because the mscv {fmt} is fixed at some higher version on compiler-explorer.
However the code below was checked working on vs2022 and gcc 4 (which is from 2014)
 
```
#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include "windows.h"

template <typename... Args>
void DebugMessage(fmt::string_view format, Args &&... args)
{
    auto formatted = fmt::vformat(format, fmt::make_args_checked<Args...>(format, std::forward<Args>(args)...));
    OutputDebugStringA(formatted.data());
}

int main()
{
    const int answer = 42;
    DebugMessage("Just a message preceeded by the current thread-id\n");
    DebugMessage("The variable '{}' contains the value '{}'.\n", "answer", answer);
    return 0;
}
```

https://godbolt.org/z/49r7Ydq17

## An {fmt} v8 or later wrapper around OutputDebugString

see: https://godbolt.org/z/58TxqhKWP

Notice that the checks of format arguments are done at compile time by `fmt::make_format_args` and because of that, you need
a compiler with constexpr support.

```
#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include "windows.h"

template <typename... Args>
void DebugMessage(fmt::string_view format, Args &&... args)
{
    auto formatted = fmt::vformat(format, fmt::make_format_args(std::forward<Args>(args)...));
    OutputDebugStringA(fmt::format("[tid {}] {}\n", ::GetCurrentThreadId(), formatted).data());
}

int main()
{
    const int answer = 42;
    DebugMessage("Just a message preceeded by the current thread-id");
    DebugMessage("The variable '{}' contains the value '{}'.", "answer", answer);
    return 0;
}
```

## An standard C++20 wrapper around OutputDebugString

If you are on C++20, fmt is included in the standard library and you can do this.
This has the advantage of having no extra library dependencies and format arguments checked at compile time.

https://godbolt.org/z/hrGW3Kn5P

```
#include <format>
#include <string_view>

#include "windows.h"

template <typename... Args>
void DebugMessage(std::string_view format_string, Args &&... args)
{
    auto formatted = std::vformat(format_string, std::make_format_args(std::forward<Args>(args)...));
    OutputDebugStringA(std::format("[tid {}] {}\n", ::GetCurrentThreadId(), formatted).data());
}

int main()
{
    const int answer = 42;
    DebugMessage("Just a message preceeded by the current thread-id\n");
    DebugMessage("The variable '{}' contains the value '{}'.\n", "answer", answer);
    return 0;
}
```

## A simple ostream wrapper

see: https://godbolt.org/z/b186hx5xM

If you are on an OLD compiler from before 2010, you can still do this,
this is old-school streaming, its slower and has no compile time checks.

```
#include <sstream>
#include <ostream>
#include <windows.h>
 
class dbgview_buffer : public std::stringbuf
{
public:
    ~dbgview_buffer() override {  sync(); }
 
    int sync()
    {
        OutputDebugString(str().c_str());
        str("");
        return 0;
    }
};

class dbgview_t : public std::ostream
{
public:
    dbgview_t() : std::ostream(&m_buf) {}
private:
    dbgview_buffer m_buf;
};
 
__declspec(selectany) dbgview_t dbgview;

int main()
{
    dbgview << "Just a message printing number " << 42;
    return 0;
}
```
