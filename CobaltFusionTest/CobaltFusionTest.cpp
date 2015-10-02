// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"

#pragma warning(disable: 4702 4389)		//ignore signed/unsigned comparision and unreachable code in boost/test

// run as CobaltFusionTest.exe --log_level=test_suite

#define BOOST_TEST_MODULE CobaltFusionLib Unit Test
#include <boost/test/unit_test_gui.hpp>
#include "CobaltFusion/CircularBuffer.h"

namespace fusion {

BOOST_AUTO_TEST_SUITE(ColbaltFusionLib)

class SynchronizedCircularBuffer
{
public:
	struct Timeout : std::runtime_error
	{
		Timeout() : std::runtime_error("SynchronizedCircularBuffer timeout")
		{
		}
	};

	explicit SynchronizedCircularBuffer(size_t capacity) : m_buffer(capacity)
	{
	}

	size_t Capacity() const
	{
		return m_buffer.Capacity();
	}

	bool Empty() const
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		return m_buffer.Empty();
	}

	bool Full() const
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		return m_buffer.Full();
	}

	size_t Available() const
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		return m_buffer.Available();
	}

	size_t Size() const
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		return m_buffer.Size();
	}
	
	char Read()
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		m_cond.wait(lock, [this] { return !m_buffer.Empty(); });
		char c = m_buffer.Read();
		lock.unlock();
		m_cond.notify_all();
		return c;
	}

	void Write(char c)
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		m_cond.wait(lock, [this] { return !m_buffer.Full(); });
		m_buffer.Write(c);
		lock.unlock();
		m_cond.notify_all();
	}

	void Clear()
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		m_buffer.Clear();
		lock.unlock();
		m_cond.notify_all();
	}

	void Swap(SynchronizedCircularBuffer& cb)
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		boost::unique_lock<boost::mutex> cbLock(cb.m_mtx);
		m_buffer.Swap(cb.m_buffer);
		cbLock.unlock();
		lock.unlock();
		cb.m_cond.notify_all();
		m_cond.notify_all();
	}

	void WriteStringZ(const std::string& s)
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		m_cond.wait(lock, [this, s] { return m_buffer.Available() > s.size(); });
		m_buffer.WriteStringZ(s.c_str());
		lock.unlock();
		m_cond.notify_all();
	}

	void WriteStringZ(const std::string& s, const boost::chrono::system_clock::duration& timeout)
	{
		boost::unique_lock<boost::mutex> lock(m_mtx);
		if (!m_cond.wait_for(lock, timeout, [this, s] { return m_buffer.Available() > s.size(); }))
			throw Timeout();
		m_buffer.WriteStringZ(s.c_str());
		lock.unlock();
		m_cond.notify_all();
	}

	std::string ReadStringZ()
	{
		std::string s;
		while (auto c = Read())
			s += c;
		return s;
	}

private:
	mutable boost::mutex m_mtx;
	mutable boost::condition_variable m_cond;
	CircularBuffer m_buffer;
};

BOOST_AUTO_TEST_CASE(CircularBufferSize)
{
	CircularBuffer buffer(100);
	CircularBuffer buffer2(2*1024*1024);
	BOOST_CHECK_EQUAL(buffer2.Capacity(), 2*1024*1024);
}

BOOST_AUTO_TEST_CASE(CircularBufferInitialLevels)
{
	size_t testsize = 100;
	CircularBuffer buffer(testsize);
	BOOST_REQUIRE_EQUAL(testsize, buffer.Capacity());
	
	BOOST_CHECK(buffer.Empty());
	BOOST_CHECK(!buffer.Full());
	for (size_t i = 0; i < buffer.Capacity(); ++i)
		buffer.Write(1);

	BOOST_CHECK(!buffer.Empty());
	BOOST_CHECK(buffer.Full());

	for (size_t i = 0; i < buffer.Capacity(); ++i)
		buffer.Read();

	BOOST_CHECK(buffer.Empty());
	BOOST_CHECK(!buffer.Full());
}

BOOST_AUTO_TEST_CASE(CircularBufferCycle)
{
	size_t testsize = 100;
	CircularBuffer buffer(testsize);
	
	for (int j = 0; j < 1500; ++j)
	{
		BOOST_CHECK(buffer.Empty());
		for (int i = 0; i < 17; ++i)
			buffer.Write(1);

		BOOST_CHECK(!buffer.Empty());

		for (int i = 0; i < 17; ++i)
			buffer.Read();

		BOOST_CHECK(buffer.Empty());
	}
}

BOOST_AUTO_TEST_CASE(CircularBufferCycleStringZ)
{
	size_t testsize = 100;
	CircularBuffer buffer(testsize);
	
	for (int j = 0; j < 1000; ++j)
	{
		BOOST_CHECK(buffer.Empty());
		for (int i = 0; i < 17; ++i)
			buffer.WriteStringZ("test");

		BOOST_CHECK(!buffer.Empty());

		for (int i = 0; i < 17; ++i)
			BOOST_CHECK_EQUAL(buffer.ReadStringZ(), "test");

		BOOST_CHECK(buffer.Empty());
	}
}

BOOST_AUTO_TEST_CASE(CircularBufferCycleStringZPrime)
{
	size_t testsize = 200;
	CircularBuffer buffer(testsize);
	
	for (int j = 0; j < 500; ++j)
	{
		BOOST_CHECK(buffer.Empty());
		for (size_t i = 0; i < 17; ++i)
			buffer.WriteStringZ("test123");

		BOOST_CHECK(!buffer.Empty());

		for (int i = 0; i < 17; ++i)
			BOOST_CHECK_EQUAL(buffer.ReadStringZ(), "test123");

		BOOST_CHECK(buffer.Empty());
	}
}

BOOST_AUTO_TEST_CASE(CircularBufferBufferFullTimeout)
{
	size_t testsize = 80;
	SynchronizedCircularBuffer buffer(testsize);
	BOOST_CHECK(buffer.Empty());
	BOOST_CHECK(!buffer.Full());

	int iterations = 0;
	int writeIterations = 0;
	BOOST_CHECK_THROW(
	{
		for (int i = 0; i < 100; ++i)
		{
			++iterations;
			buffer.WriteStringZ("test123", boost::chrono::seconds(1));
			++writeIterations;
			BOOST_CHECK(!buffer.Empty());
		}
	}, std::exception);

	BOOST_MESSAGE("iterations: " << iterations);
	BOOST_CHECK(buffer.Full());

	int readInterations = 0;
	while (!buffer.Empty())
	{
		buffer.ReadStringZ();
		++readInterations;
	}
	BOOST_CHECK(buffer.Empty());
	BOOST_CHECK(!buffer.Full());
	BOOST_CHECK_EQUAL(readInterations, writeIterations);
}

BOOST_AUTO_TEST_CASE(CircularBufferSwapping)
{
	size_t testsize = 30;
	CircularBuffer buffer(testsize);

	size_t testsize2 = 60;
	CircularBuffer buffer2(testsize2);

	buffer.WriteStringZ("test");
	buffer.WriteStringZ("test");
	buffer2.WriteStringZ("test");

	BOOST_REQUIRE_EQUAL(buffer.Size(), 10);
	BOOST_REQUIRE_EQUAL(buffer2.Size(), 5);

	buffer.Swap(buffer2);

	BOOST_REQUIRE_EQUAL(buffer.Size(), 5);
	BOOST_REQUIRE_EQUAL(buffer2.Size(), 10);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace fusion
