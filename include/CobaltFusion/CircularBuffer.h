// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <memory>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#pragma comment(lib, "CobaltFusion.lib")

namespace fusion {

class CircularBuffer : boost::noncopyable
{
public:
	explicit CircularBuffer(size_t size);
	virtual ~CircularBuffer();
	size_t Size() const;

	virtual bool Empty() const;
	virtual bool Full() const;
	virtual size_t GetFree() const;
	virtual size_t GetCount() const;

	char Read();
	std::string ReadStringZ();
	void Write(char c);
	void WriteStringZ(const char* message);

	void Clear();
	void Swap(CircularBuffer& circularBuffer);
	void DumpStats();

protected:
    void NotifyWriter();

	size_t NextPosition(size_t offset) const
	{
		return offset+1 == m_size ? 0 : offset+1;
	}

	const char* ReadPointer() const
	{
		return m_buffer.get() + m_readOffset;
	}

	char* WritePointer() const
	{
		return m_buffer.get() + m_writeOffset;
	}

	void IncreaseReadPointer()
	{
		m_readOffset = NextPosition(m_readOffset);
	}

	void IncreaseWritePointer()
	{
		m_writeOffset = NextPosition(m_writeOffset);
	}

	static int GetPowerOfTwo(int size);
	void WaitForReader();
	virtual void WaitForReaderTimeout();

private:
	void AssignBuffer(std::unique_ptr<char> buffer, size_t size, size_t readOffset, size_t writeOffset);

	size_t m_size;						// important: m_buffer must be initialized after m_size
	std::unique_ptr<char> m_buffer;		//
	size_t m_readOffset;
	size_t m_writeOffset;
	boost::condition_variable m_triggerRead;
};



} // namespace fusion
