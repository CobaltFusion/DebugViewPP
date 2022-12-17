
# An {fmt} wrapper around OutputDebugString makes sending formatted message much more plesant.
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
