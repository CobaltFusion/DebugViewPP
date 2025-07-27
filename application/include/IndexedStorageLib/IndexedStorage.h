// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <vector>
#include <string>
#include <limits>

namespace fusion {
namespace indexedstorage {

inline constexpr size_t UNSET_VALUE = std::numeric_limits<size_t>::max();

class VectorStorage
{
public:
    [[nodiscard]] bool Empty() const;
    void Clear();
    size_t Add(const std::string& value);
    [[nodiscard]] size_t Count() const;
    std::string operator[](size_t i) const;
    void shrink_to_fit();

private:
    std::vector<std::string> m_storage;
};

class SnappyStorage
{
public:
    SnappyStorage() = default;

    [[nodiscard]] bool Empty() const;
    void Clear();
    size_t Add(const std::string& value);
    [[nodiscard]] size_t Count() const;
    std::string operator[](size_t i);

    [[nodiscard]] std::string Compress(const std::vector<std::string>& value) const;
    static std::vector<std::string> Decompress(const std::string& value);
    void shrink_to_fit();

private:
    static size_t GetBlockIndex(size_t index);
    static size_t GetRelativeIndex(size_t index);
    std::string GetString(size_t index);

    size_t m_writeBlockIndex = 0;
    size_t m_readBlockIndex = UNSET_VALUE;
    std::vector<std::string> m_readList;
    std::vector<std::string> m_writeList;
    std::vector<std::string> m_storage;
};

} // namespace indexedstorage
} // namespace fusion
