// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"

#pragma warning(disable: 4702 4389)		//ignore signed/unsigned comparision and unreachable code in boost/test

#define BOOST_TEST_MODULE CobaltFusionLib Unit Test
#include <boost/test/unit_test_gui.hpp>
#include "CobaltFusion/CircularBuffer.h"


namespace fusion {

BOOST_AUTO_TEST_SUITE(ColbaltFusionLib)

class TestCircularBuffer : public CircularBuffer
{
public:
	explicit TestCircularBuffer(size_t size) : CircularBuffer(size)
	{
	}

	virtual bool Full() const
	{
		return CircularBuffer::GetFree() < 10;
	}

	void TestBlockingWriteString()
	{
		if (Full())
			WaitForReader();
		WriteStringZ("test123");
	}

	std::string TestReadString()
	{
		auto message = ReadStringZ();
		NotifyWriter();
		return message;
	}
};

BOOST_AUTO_TEST_CASE(CircularBufferSize)
{
	CircularBuffer buffer(100);
	BOOST_CHECK_EQUAL(128, buffer.Size());

	CircularBuffer buffer2(2*1024*1024);
	BOOST_CHECK_EQUAL(buffer2.Size(), 2*1024*1024);
}

BOOST_AUTO_TEST_CASE(CircularBufferInitialLevels)
{
	size_t testsize = 100;
	CircularBuffer buffer(testsize);
	BOOST_REQUIRE_EQUAL(128, buffer.Size());
	
	BOOST_CHECK(buffer.Empty());
	BOOST_CHECK(!buffer.Full());
	for (size_t i = 0; i < buffer.Size() - 1; ++i)
		buffer.Write<char>(1);

	BOOST_CHECK(!buffer.Empty());
	BOOST_CHECK(buffer.Full());

	for (size_t i = 0; i < buffer.Size() - 1; ++i)
		buffer.Read<char>();

	BOOST_CHECK(buffer.Empty());
	BOOST_CHECK(!buffer.Full());
}

BOOST_AUTO_TEST_CASE(CircularBufferCycle)
{
	size_t testsize = 100;
	CircularBuffer buffer(testsize);
	BOOST_REQUIRE_EQUAL(128, buffer.Size());
	
	for (int j = 0; j < 1500; ++j)
	{
		BOOST_CHECK(buffer.Empty());
		for (int i = 0; i < 17; ++i)
			buffer.Write<char>(1);

		BOOST_CHECK(!buffer.Empty());

		for (int i = 0; i < 17; ++i)
			buffer.Read<char>();

		BOOST_CHECK(buffer.Empty());
	}
}

BOOST_AUTO_TEST_CASE(CircularBufferCycleStringZ)
{
	size_t testsize = 100;
	CircularBuffer buffer(testsize);
	BOOST_CHECK_EQUAL(buffer.Size(), 128);
	
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
	BOOST_REQUIRE_EQUAL(256, buffer.Size());
	
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
	size_t testsize = 100;
	TestCircularBuffer buffer(testsize);
	BOOST_CHECK_EQUAL(buffer.Size(), 128);
	BOOST_CHECK(buffer.Empty());
	BOOST_CHECK(!buffer.Full());

	int iterations = 0;
	int writeIterations = 0;
	BOOST_CHECK_THROW(
	{
		for (int i = 0; i < 100; ++i)
		{
			++iterations;
			buffer.TestBlockingWriteString();
			++writeIterations;
			BOOST_CHECK(!buffer.Empty());
		}
	}, std::exception);

	BOOST_MESSAGE("iterations: " << iterations);
	BOOST_CHECK(buffer.Full());

	int readInterations = 0;
	while (!buffer.Empty())
	{
		buffer.TestReadString();
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
	BOOST_REQUIRE_EQUAL(buffer.Size(), 32);

	size_t testsize2 = 60;
	CircularBuffer buffer2(testsize2);
	BOOST_REQUIRE_EQUAL(buffer2.Size(), 64);

	buffer.WriteStringZ("test");
	buffer.WriteStringZ("test");
	buffer2.WriteStringZ("test");

	BOOST_REQUIRE_EQUAL(buffer.GetCount(), 10);
	BOOST_REQUIRE_EQUAL(buffer2.GetCount(), 5);

	buffer.Swap(buffer2);

	BOOST_REQUIRE_EQUAL(buffer.GetCount(), 5);
	BOOST_REQUIRE_EQUAL(buffer2.GetCount(), 10);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace fusion
