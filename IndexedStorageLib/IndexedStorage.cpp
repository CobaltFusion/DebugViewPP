// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <vector>
#include "IndexedStorageLib/IndexedStorage.h"
#include "../libsnappy/libsnappy.h"

namespace fusion {
namespace indexedstorage {

const int blockSize = 400;

bool VectorStorage::Empty() const
{
	return m_storage.empty();
}

void VectorStorage::Clear()
{
	m_storage.clear();
	m_storage.shrink_to_fit();
}

size_t VectorStorage::Add(const std::string& value)
{
	m_storage.push_back(value);
	return m_storage.size() - 1;
}

size_t VectorStorage::Count() const
{
	return m_storage.size();
}

std::string VectorStorage::operator[](size_t i) const
{
	return m_storage[i];
}

SnappyStorage::SnappyStorage() :
	m_writeBlockIndex(0),
	m_readBlockIndex(-1)
{
}

bool SnappyStorage::Empty() const
{
	return m_storage.empty();
}

void SnappyStorage::Clear()
{
	m_storage.clear();
	m_storage.shrink_to_fit();

	m_readList.clear();
	m_readList.shrink_to_fit();
	m_readBlockIndex = -1;

	m_writeList.clear();
	m_writeList.shrink_to_fit();
	m_writeBlockIndex = 0;
}

size_t SnappyStorage::Add(const std::string& value)
{
	auto id = m_writeList.size();
	m_writeList.push_back(value);
	auto result = m_writeBlockIndex * blockSize + id;
	if (id == blockSize - 1)
	{
		std::string test = Compress(m_writeList);
		m_storage.push_back(test);
		m_writeList.clear();
		++m_writeBlockIndex;
	}
	return result;
}

size_t SnappyStorage::Count() const
{
	return m_storage.size();
}

std::string SnappyStorage::operator[](size_t i)
{
	return GetString(i);
}

size_t SnappyStorage::GetBlockIndex(size_t index)
{
	return index / blockSize;
}

size_t SnappyStorage::GetRelativeIndex(size_t index)
{
	return index % blockSize;
}

std::string SnappyStorage::GetString(size_t index)
{
	auto blockId = GetBlockIndex(index);
	auto id = GetRelativeIndex(index);

	if (blockId == m_writeBlockIndex)
	{
		return m_writeList[id];
	}

	if (blockId != m_readBlockIndex)
	{
		m_readList = Decompress(m_storage[blockId]);
		m_readBlockIndex = blockId;
	}
	return m_readList[id];
}

std::string SnappyStorage::Compress(const std::vector<std::string>& value) const
{
	std::vector<char> raw;
	for (auto& s : value)
	{
		for (auto t : s)
		{
			raw.push_back(t);
		}
		raw.push_back('\0');
	}
	std::string data;
	snappy::Compress(&raw[0], raw.size(), &data);
	return data;
}

std::vector<std::string> SnappyStorage::Decompress(const std::string& value)
{
	std::vector<std::string> vec;

	std::string data;
	snappy::Uncompress(value.c_str(), value.size(), &data);

	for (auto it = data.begin(); it != data.end(); ++it)
	{
		auto begin = it;
		while (*it)
			++it;		// cppcheck : Message: The iterator incrementing is suspicious - it is incremented at line 150 and then at line 146. The loop might unintentionally skip an element in the container. There is no comparison between these increments to prevent that the iterator is incremented beyond the end.
		vec.emplace_back(begin, it);
	}
	return vec;
}

void SnappyStorage::shrink_to_fit()
{
	m_readList.shrink_to_fit();
	m_writeList.shrink_to_fit();
	m_storage.shrink_to_fit();
}

} // namespace indexedstorage
} // namespace fusion
