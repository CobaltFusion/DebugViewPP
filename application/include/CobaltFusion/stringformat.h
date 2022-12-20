// (C) Copyright Jan Wilmans 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <string>

namespace fusion {

[[nodiscard]] std::vector<std::string> split(std::string_view value, char separator);
[[nodiscard]] std::string join(const std::vector<std::string>& values, const std::string& separator);

static constexpr const char whitespace[] = "\t\n\v\f\r ";
// remove characters from beginning or end of a string, or both
[[nodiscard]] std::string ltrim(std::string str, const std::string& chars = whitespace);
[[nodiscard]] std::string rtrim(std::string str, const std::string& chars = whitespace);
[[nodiscard]] std::string strip(std::string str, const std::string& chars = whitespace);

} // namespace fusion
