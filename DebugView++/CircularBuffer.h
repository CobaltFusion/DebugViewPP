// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

namespace fusion {
namespace debugviewpp {

class CircularBuffer : boost::noncopyable
{
public:
	explicit CircularBuffer(size_t size);
	~CircularBuffer();

private:
	char* m_pBegin;
	char* m_pEnd;
	const char* m_pRead;
	char* m_pWrite;
};

} // namespace debugviewpp 
} // namespace fusion
