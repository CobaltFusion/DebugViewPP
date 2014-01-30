// DebugViewPPTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "gtest/gtest.h"
#include <random>

#ifdef _DEBUG
#        pragma comment(lib, "gtestd.lib")
#else
#        pragma comment(lib, "gtest.lib")
#endif

#include "../IndexedStorageLib/IndexedStorageLib.h"
#include "../Win32Lib/Win32Lib.h"
#include "../DBWinBufferLib/DBWinBufferLib.h"

using namespace fusion;
using namespace fusion::indexedstorage;
using namespace fusion::debugviewpp;

std::string GetTestString(int i)
{
	return stringbuilder() << "BB_TEST_ABCDEFGHI_EE_" << i;
}

TEST(TestCase, IndexedStorageRandomAccess)
{
	size_t testSize = 10000;
	auto testMax = testSize -1;

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, testMax);
	SnappyStorage s;
	for (size_t i=0; i< testSize; ++i)
		s.Add(GetTestString(i));

	for (size_t i=0; i< testSize; ++i)
	{
		size_t j = distribution(generator);  // generates number in the range 0..testMax 
		EXPECT_EQ(s[j], GetTestString(j));
	}
}

TEST(TestCase, IndexedStorageCompression)
{
	size_t testSize = 10000;
	auto testMax = testSize -1;

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, testMax);
	SnappyStorage s;
	VectorStorage v;

	size_t m0 = ProcessInfo::GetPrivateBytes();

	for (size_t i=0; i< testSize; ++i)
		v.Add(GetTestString(i));

	size_t m1 = ProcessInfo::GetPrivateBytes();
	size_t required1 = m1-m0;
	std::cout << "VectorStorage requires: " << required1/1024 << " bK\n";

	for (size_t i=0; i< 100000; ++i)
		s.Add(GetTestString(i));

	size_t m2 = ProcessInfo::GetPrivateBytes();
	size_t required2 = m2-m1;
	std::cout << "SnappyStorage requires: " << required2/1024 << " kB (" << (100*required2)/required1 << "%)\n";
	EXPECT_GT(required1/2, required2) << "Expected a compression ratio below 50%!";
}

int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}