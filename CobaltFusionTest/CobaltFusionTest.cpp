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

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}