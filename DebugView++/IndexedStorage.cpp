// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "IndexedStorage.h"
#include "libsnappy.h"
#include "Utilities.h"
#include <vector>

namespace fusion {
	
	const int blockSize = 400;

	VectorStorage::VectorStorage()
	{
	}

	VectorStorage::~VectorStorage()
	{
	}

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

	SnappyStorage::~SnappyStorage()
	{
	}

	bool SnappyStorage::Empty() const
	{
		return m_storage.empty();
	}

	void SnappyStorage::Clear()
	{
		m_storage.clear();
	}

	int SnappyStorage::Add(const std::string& value)
	{
		int id = m_writeList.size();
		m_writeList.push_back(value);
		int result = (m_writeBlockIndex* blockSize) + id;
        if (id == (blockSize-1))
        {
            m_storage[m_writeBlockIndex] = Compress(m_writeList);
            m_writeBlockIndex++;
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
        return (index / blockSize);
    }

    int SnappyStorage::GetRelativeIndex(int index) const
    {
        return (index % blockSize);
    }

    std::string SnappyStorage::GetString(int index)
    {
        int blockId = GetBlockIndex(index);
        int id = GetRelativeIndex(index);
        
		if (blockId == m_writeBlockIndex)
        {
            return m_writeList[id];
        }

        if (m_readBlockIndex != blockId)
        {
            m_readList = Decompress(m_storage[blockId]);
            m_readBlockIndex = blockId;
        }
        return m_readList[id];
    }

	std::string SnappyStorage::Compress(std::vector<std::string> value) const
	{
		auto sb = stringbuilder();
		for (auto s = value.begin(); s != value.end(); s++)
		{
			sb << s->c_str();
			sb << "\0";
		}
		std::string data;
		snappy::Compress(sb.str().c_str(), sb.str().size(), &data);
		return std::move(data);
	}

	std::vector<std::string> SnappyStorage::Decompress(const std::string& value) const
	{
		std::vector<std::string> vec;

		std::string data;
		snappy::Uncompress(value.c_str(), value.size(), &data);

		for (auto i = data.begin(); i != data.end(); i++)
		{
			auto sb = stringbuilder();
			while (*i)
			{
				sb << *i;
			}
			vec.push_back(sb.str());
		}
		return std::move(vec);
	}

} // namespace fusion

