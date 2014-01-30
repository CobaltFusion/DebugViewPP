// DebugViewPPTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "gtest/gtest.h"

#ifdef _DEBUG
#        pragma comment(lib, "gtestd.lib")
#else
#        pragma comment(lib, "gtest.lib")
#endif

#include "../IndexedStorageLib/IndexedStorageLib.h"

using namespace fusion::indexedstorage;

TEST(TestCase, IndexedStorage)
{
	SnappyStorage s;
}

int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}