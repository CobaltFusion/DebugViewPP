// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <sstream>
#include <cstddef>

namespace fusion {

// takes an address and a size and return the hexadecimal values of the bytes as "[xx xx ...]"
template <typename T>
auto to_hex(const T* value, size_t size_in_bytes)
{
    std::ostringstream ss;

    auto to_hex = [&ss](std::byte value) -> std::ostringstream& {
        ss << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(value);
        return ss;
    };

    ss << "[";
    const std::byte* data = reinterpret_cast<const std::byte*>(value);
    size_t lastIndex = size_in_bytes - 1;
    for (size_t i = 0; i < lastIndex; ++i)
    {
        to_hex(data[i]) << " ";
    }
    to_hex(data[lastIndex]) << "]";
    return ss.str();
};

template <typename T>
auto to_hex(const T& container)
{
    return to_hex(container.data(), container.size() * sizeof(container[0]));
}
} // namespace fusion
