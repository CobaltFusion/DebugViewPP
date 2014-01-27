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
	EXPECT_EQ(buffer.Size(), 128);

    fusion::CircularBuffer buffer2(2*1024*1024);
	EXPECT_EQ(buffer2.Size(), 2*1024*1024);
}


int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}