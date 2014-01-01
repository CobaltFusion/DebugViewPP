// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <vector>
#include "IndexedStorage.h"
#include "libsnappy.h"
#include "Utilities.h"

namespace fusion {
namespace debugviewpp {
	
const int blockSize = 400;

bool VectorStorage::Empty() const
{
	return m_storage.empty();
}

void VectorStorage::Clear()
{
	m_storage.clear();
}

int VectorStorage::Add(const std::string& value)
{
	m_storage.push_back(value);
	return m_storage.size() - 1;
}

int VectorStorage::Count() const
{
	return m_storage.size();
}

std::string VectorStorage::operator[](int i) const
{
	return m_storage[i];
}

SnappyStorage::SnappyStorage() :
	m_readBlockIndex(-1),
	m_writeBlockIndex(0)
{
}

bool SnappyStorage::Empty() const
{
	return m_storage.empty();
}

void SnappyStorage::Clear()
{
	m_storage.clear();
	m_writeList.clear();
	m_writeBlockIndex = 0;
}

int SnappyStorage::Add(const std::string& value)
{
	int id = m_writeList.size();
	m_writeList.push_back(value);
	int result = m_writeBlockIndex * blockSize + id;
	if (id == blockSize - 1)
	{
		std::string test = Compress(m_writeList);
		m_storage.push_back(test);
		m_writeList.clear();
		++m_writeBlockIndex;
	}
	return result;
}

int SnappyStorage::Count() const
{
	return m_storage.size();
}

std::string SnappyStorage::operator[](int i)
{
	return std::move(GetString(i));
}

int SnappyStorage::GetBlockIndex(int index) const
{
	return index / blockSize;
}

int SnappyStorage::GetRelativeIndex(int index) const
{
	return index % blockSize;
}

std::string SnappyStorage::GetString(int index)
{
	int blockId = GetBlockIndex(index);
	int id = GetRelativeIndex(index);
        
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
	for (auto s = value.begin(); s != value.end(); ++s)
	{
		for (auto t = s->begin(); t != s->end(); ++t)
		{
			raw.push_back(*t);
		}
		raw.push_back('\0');
	}
	std::string data;
	snappy::Compress(&raw[0], raw.size(), &data);
	return data;
}

std::vector<std::string> SnappyStorage::Decompress(const std::string& value) const
{
	std::vector<std::string> vec;

	std::string data;
	snappy::Uncompress(value.c_str(), value.size(), &data);

	for (auto it = data.begin(); it != data.end(); ++it)
	{
		auto begin = it;
		while (*it)
			++it;
		vec.push_back(std::string(begin, it));
	}
	return vec;
}

} // namespace debugviewpp 
} // namespace fusion
