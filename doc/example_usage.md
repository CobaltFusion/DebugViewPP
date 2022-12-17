
# An {fmt} wrapper around OutputDebugString

This makes sending formatted message much more plesant.

see: https://godbolt.org/z/75aq9Ef58

 
```
#include <fmt/format.h>

#include <sstream>
#include <string_view>

void OutputDebugString(const char * message)
{
    fmt::print("{}", message);
}

void DebugMessage(std::string_view message)
{
    OutputDebugString(message.data());
}

template <typename... Args>
void DebugMessage(std::string_view format, Args &&... args)
{
    auto formatted = fmt::vformat(format, fmt::make_args_checked<Args...>(format, std::forward<Args>(args)...));
    OutputDebugString(formatted.data());
}

int main()
{
    DebugMessage("test");
    DebugMessage("[{}]", 42);
    return 0;
}
```

# A simple ostream wrapper

see: https://godbolt.org/z/b186hx5xM

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
