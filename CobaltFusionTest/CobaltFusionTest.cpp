// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "gtest/gtest.h"
#include "cobaltfusion.h"

#ifdef _DEBUG
#        pragma comment(lib, "gtestd.lib")
#else
#        pragma comment(lib, "gtest.lib")
#endif


class TestCircularBuffer : public fusion::CircularBuffer
{
public:
	TestCircularBuffer(size_t size) : fusion::CircularBuffer(size)
	{
	}

	virtual bool Full() const
	{
		return fusion::CircularBuffer::GetFree() < 10;
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


TEST(TestCase, Test)
{
	EXPECT_EQ(1 + 1, 2);
}

TEST(TestCase, CircularBufferSize)
{
	fusion::CircularBuffer buffer(100);
	EXPECT_EQ(128, buffer.Size());

	fusion::CircularBuffer buffer2(2*1024*1024);
	EXPECT_EQ(2*1024*1024, buffer2.Size());
}

TEST(TestCase, CircularBufferInitialLevels)
{
	size_t testsize = 100;
	fusion::CircularBuffer buffer(testsize);
	EXPECT_EQ(128, buffer.Size());
	
	EXPECT_EQ(true, buffer.Empty());
	EXPECT_EQ(false, buffer.Full());
	for (size_t i=0; i< buffer.Size()-1; ++i)
	{
		buffer.Write(char(1));
	}
	EXPECT_EQ(false, buffer.Empty());
	EXPECT_EQ(true, buffer.Full());

	for (size_t i=0; i< buffer.Size()-1; ++i)
	{
		buffer.Read<char>();
	}
	EXPECT_EQ(true, buffer.Empty());
	EXPECT_EQ(false, buffer.Full());
}

TEST(TestCase, CircularBufferCycle)
{
	size_t testsize = 100;
	fusion::CircularBuffer buffer(testsize);
	EXPECT_EQ(128, buffer.Size());
	
	for (int j=0; j<1500; ++j)
	{
		//std::cout << "Test Cycle Number " << j+1 << std::endl;
		EXPECT_EQ(true, buffer.Empty());
		for (size_t i=0; i< 17; ++i)
		{
			buffer.Write(char(1));
		}
		EXPECT_EQ(false, buffer.Empty());

		for (size_t i=0; i<17; ++i)
		{
			buffer.Read<char>();
		}
		EXPECT_EQ(true, buffer.Empty());
	}
}

TEST(TestCase, CircularBufferCycleStringZ)
{
	size_t testsize = 100;
	fusion::CircularBuffer buffer(testsize);
	EXPECT_EQ(128, buffer.Size());
	
	for (int j=0; j<1000; ++j)
	{
		//std::cout << "Test Cycle Number " << j+1 << std::endl;
		EXPECT_EQ(true, buffer.Empty());
		for (size_t i=0; i< 17; ++i)
		{
			buffer.WriteStringZ("test");
		}
		EXPECT_EQ(false, buffer.Empty());

		for (size_t i=0; i<17; ++i)
		{
			auto test = buffer.ReadStringZ();
			EXPECT_EQ("test", test);
		}
		EXPECT_EQ(true, buffer.Empty());
	}
}

TEST(TestCase, CircularBufferCycleStringZPrime)
{
	size_t testsize = 200;
	fusion::CircularBuffer buffer(testsize);
	EXPECT_EQ(256, buffer.Size());
	
	for (int j=0; j<500; ++j)
	{
		//std::cout << "Test Cycle Number " << j+1 << std::endl;
		EXPECT_EQ(true, buffer.Empty());
		for (size_t i=0; i< 17; ++i)
		{
			buffer.WriteStringZ("test123");
		}
		EXPECT_EQ(false, buffer.Empty());

		for (size_t i=0; i<17; ++i)
		{
			auto test123 = buffer.ReadStringZ();
			EXPECT_EQ("test123", test123);
		}
		EXPECT_EQ(true, buffer.Empty());
	}
}

TEST(TestCase, CircularBufferBufferFullTimeout)
{
	size_t testsize = 100;
	TestCircularBuffer buffer(testsize);
	EXPECT_EQ(128, buffer.Size());
	EXPECT_EQ(true, buffer.Empty());
	EXPECT_EQ(false, buffer.Full());

	bool gotException = false;
	int iterations = 0;
	int writeIterations = 0;
	try {
		for (size_t i=0; i< 100; ++i)
		{
			iterations++;
			buffer.TestBlockingWriteString();
			writeIterations++;
			EXPECT_EQ(false, buffer.Empty());
		}
	}
	catch (std::exception& e)
	{
		std::cout << ">> as expected, got exception: " << e.what() << std::endl;
		gotException = true;
	}
	std::cout << "iterations: " << iterations << std::endl;
	EXPECT_EQ(true, gotException);
	EXPECT_EQ(true, buffer.Full());

	int readInterations = 0;
	while (!buffer.Empty())
	{
		buffer.TestReadString();
		readInterations++;
	}
	EXPECT_EQ(true, buffer.Empty());
	EXPECT_EQ(false, buffer.Full());
	EXPECT_EQ(writeIterations, readInterations);
}

TEST(TestCase, CircularBufferSwapping)
{
	size_t testsize = 30;
	fusion::CircularBuffer buffer(testsize);
	EXPECT_EQ(32, buffer.Size());

	size_t testsize2 = 60;
	fusion::CircularBuffer buffer2(testsize2);
	EXPECT_EQ(64, buffer2.Size());

	buffer.WriteStringZ("test");
	buffer.WriteStringZ("test");
	buffer2.WriteStringZ("test");

	EXPECT_EQ(10, buffer.GetCount());
	EXPECT_EQ(5, buffer2.GetCount());

	buffer.Swap(buffer2);

	EXPECT_EQ(5, buffer.GetCount());
	EXPECT_EQ(10, buffer2.GetCount());

}

int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}