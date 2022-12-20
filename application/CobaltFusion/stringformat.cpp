// (C) Copyright Jan Wilmans 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <string>

namespace fusion {

std::vector<std::string> split(std::string_view value, char separator)
{
    std::vector<std::string> result;
    std::size_t start = 0;
    auto current = start;

    while ((current = value.find(separator, start)) != std::string::npos)
    {
        auto view = value.substr(start, current - start);
        if (view.begin() != view.end())
        {
            result.emplace_back(view.begin(), view.end());
        }
        start = current + 1;
    }
    auto view = value.substr(start);
    if (view.begin() != view.end())
    {
        result.emplace_back(view.begin(), view.end());
    }
    return result;
}

std::string join(const std::vector<std::string>& values, const std::string& separator)
{
    std::string result;
    for (const auto& value : values)
    {
        result.append(value);
        result.append(separator);
    }
    auto end = result.size() - separator.size();
    return result.substr(0, end);
}

std::string ltrim(std::string str, const std::string& chars)
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}

std::string rtrim(std::string str, const std::string& chars)
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

std::string strip(std::string str, const std::string& chars)
{
    str.erase(0, str.find_first_not_of(chars));
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

} // namespace fusion
