// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "ProcessInfo.h"
#include <vector>

namespace fusion {
namespace debugviewpp {

class VectorStorage
{
public:
	bool Empty() const;
	void Clear();
	int Add(const std::string& value);
	int Count() const;
	std::string operator[](int i) const;

private:
	std::vector<std::string> m_storage;
};

class SnappyStorage
{
public:
	SnappyStorage();

	bool Empty() const;
	void Clear();
	int Add(const std::string& value);
	int Count() const;
	std::string operator[](int i);

	std::string Compress(const std::vector<std::string>& value) const;
	std::vector<std::string> Decompress(const std::string& value) const;

private:
	int GetBlockIndex(int index) const;
	int GetRelativeIndex(int index) const;
	std::string GetString(int index);

	int m_writeBlockIndex;
	int m_readBlockIndex;
	std::vector<std::string> m_readList;
	std::vector<std::string> m_writeList;

	std::vector<std::string> m_storage;
};
	
} // namespace debugviewpp 
} // namespace fusion
