// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#include <sys/timeb.h>
#include "dbgstream.h"

void DbgMsgSrc(double freq)
{
	unsigned t0 = GetTickCount();
	unsigned count = 0;
	double msgPerTick = freq * 1e-3;
	for (;;)
	{
		Sleep(10);
		while (count < (GetTickCount() - t0)*msgPerTick)
		{
			if (count % 10 == 0)
				cdbg << "Een, twee, drie, vier, vijf, zes, zeven, acht, negen, tien, elf, twaalf, dertien, veertien, vijftien, zestien, zeventien, achttien, negentien, twintig, eenentwintig, tweeentwintig, drieentwintig, vierentwintig, vijfentwintig, zesentwintig, zevenentwintig, achtentwintig, negenentwintig, dertig.\n";
			else
				cdbg << "Message #" << count << "\n";
			++count;
		}
	}
}

void DbgMsgTest()
{
	OutputDebugStringA("Message 1 without newline");
	OutputDebugStringA("Message 2 without newline");
	OutputDebugStringA("Message 3 with newline\n");
	OutputDebugStringA("Message with\nembedded newline\n");
	OutputDebugStringA("This should look like a table in a non-proportionally spaced font like 'Courier'");
	OutputDebugStringA("Colomn1\tColomn2\tColomn3\tColomn4\tColomn5");
	OutputDebugStringA("1\t\t2\t\t3\t\t4\t\t5");
	OutputDebugStringA("A\t\tB\t\tC\t\tD\t\tE");
	OutputDebugStringA("11\t\t12\t\t13\t\t14\t\t15");
	OutputDebugStringA("\t\t22\t\t23A\t\t24\t\t25");
	OutputDebugStringA("\t\t\t\t33\t\t34\t\t35");
	OutputDebugStringA("LongLine: Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Message ends HERE.\n");
}

void DbgMsgClearTest()
{
	char buffer[200];

	int line = 1;
	for (int i=0; i< 400; ++i)
	{
		sprintf_s(buffer, "Message %d", line);
		OutputDebugStringA(buffer);
		++line;
	}
	
	OutputDebugStringA("    ----> DBGVIEWCLEAR\n");

	for (int i=0; i< 10; ++i)
	{
		sprintf_s(buffer, "Message %d", line);
		OutputDebugStringA(buffer);
		++line;
	}
}

int getMilliCount()
{
	timeb tb;
	ftime(&tb);
	return tb.millitm + (tb.time & 0xfffff) * 1000;
}

void testLongString()
{
	int length = 15000;
	std::ostringstream ss;
	ss << "1234567890ABCDEF-%s {0} \t\r\n";
	ss << length;
	std::string test = ss.str();
	for (size_t i = 0; i < length - test.size() - 1; ++i)
		ss << "X";

	ss << "\n";
	test = ss.str();	
	
	OutputDebugStringA(test.c_str());
}

void Output(const std::string& filename)
{
	std::cout << "Buggazer tester, PID: " << GetCurrentProcessId() << "\n";
	std::fstream fs;
	fs.open(filename, std::fstream::in);

	if (!fs)
	{
		std::cout << "\"" << filename << "\" not found!\n";
		return;
	}

	std::cout << "Reading " << filename << "...\n";
	std::vector<std::string> lines;
	std::string line;
	while (getline(fs, line))
		lines.push_back(line);
	fs.close();

	std::cout << "writing... " << lines.size() << " lines\n";

	int i = 0;
	long t1 = getMilliCount();

	for (auto s = lines.begin(); s != lines.end(); ++s)
	{
		++i;
		//auto t = s + "\n";
		OutputDebugStringA(s->c_str());
		//Sleep(50);
	}
	long t2 = getMilliCount();

	std::cout << "OutputDebugStringA " << i << " lines, took: " << t2 - t1 << " ms\n";
}

void EndlessTest()
{
	int length = 40;		//260
	std::ostringstream ss;
	ss << "123456:jan-7890_ABC__DEF-te m_test";
	ss << length;
	std::string test = ss.str();
	for (size_t i = 0; i < length - test.size() - 1; ++i)
		ss << "X";

	ss << "\n";
	test = ss.str();

	//testLongString();

	for (;;)
	{
		long t1 = getMilliCount();

		for (int a = 0; a < 5; ++a)
		{
			OutputDebugStringA("    ## before me ##\n");

			// write exactly 2MB to the debug buffer;
			for (int i = 0; i < 9; ++i)
				OutputDebugStringA(test.c_str());

			long t2 = getMilliCount();
			OutputDebugStringA("    ## tracking me ##\n");
			std::cout << "took: " << t2 - t1 << " ms\n";
			Sleep(50);
		}
		//OutputDebugStringA("    ----> DBGVIEWCLEAR\n");
	}
}

void SeparateProcessTest()
{
	std::cerr << "SeparateProcessTest\n";
	for (;;)
	{
		auto result = (int) ShellExecute(0, L"open", L"DbgMsgSrc.exe", L"-n", nullptr, SW_HIDE);
		if (result <= 32)
		{
			std::cerr << "error starting DbgMsgSrc.exe\n";
			break;
		}
		Sleep(100);
	}
}

void CoutCerrTest()
{
    std::cout << "Message on cout 1\n";
    std::cout << "Message on cout 2\n";
    std::cerr << "Message on cerr 1\n";
    std::cerr << "Message on cerr 2\n";
    std::cout << "Message on cout 3\n";
    std::cerr << "Message on cerr 3\n";
    std::cout << "Message on cout 4\n";
    std::cerr << "Message on cerr 4\n";
    std::cout << "Message on cout 5\n";
    std::cerr << "Message on cerr 5\n";
}

void PrintUsage()
{
	std::cout <<
		"Usage: DbgMsgSrc <opt>\n"
		"\n"
		"  -1 read 'titan_crash_debugview_43mb.log' and output it through OutputDebugStringA\n"
		"  -2 <filename> read <filename> and output it through OutputDebugStringA\n"
		"  -3 run endless test\n"
		"  -s run -n repeatedly (10x / second) in separate processes)\n"
		"  -w Send OutputDebugStringA 'WithoutNewLine'\n"
		"  -n Send OutputDebugStringA 'WithNewLine\\n'\n"
		"  -e Send empty OutputDebugStringA message (does not trigger DBwinMutex!)\n"
		"  -4 Send 2x OutputDebugStringA 'WithNewLine\\n' (process handle cache test)\n"
		"  -5 Send OutputDebugStringA '1\\n2\\n3\\n'\n"
		"  -6 Send OutputDebugStringA '1 ' '2 ' '3\\n' in separate messages\n"
		"  -7 DbgMsgTest, sends 5 different test lines, using different newlines styles\n"
		"  -8 <frequency> DbgMsgSrc, Send OutputDebugStringA test lines with the specified frequency\n"
		"  -9 DBGVIEWCLEAR test\n"
		"  -A cout/cerr test\n";
}

int main(int argc, char* argv[])
{
	//OutputDebugStringA("ping");
	//return 0;

	// get un-spoofable executable file name 
	//char buf[260];
	//GetMappedFileName(GetCurrentProcess(), _tmain, buf, sizeof(buf));
	//printf("%S\n", buf);

	//HANDLE handle1 = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	//HANDLE handle2 = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	//HANDLE handle3 = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	//printf(" %p, %p, %p\n", handle1, handle2, handle3);

	int lastArgc = argc - 1;
	for (int i = 1; i < argc; ++i)
	{
		std::string arg(argv[i]);
		if (arg == "-1")
		{
			Output("titan_crash_debugview_43mb.log");
			return 0;
		}
		else if (arg == "-2")
		{
			if (i == lastArgc)
			{
				PrintUsage();
				return -1;
			}
			Output(argv[i + 1]);
			return 0;
		}
		else if (arg == "-3")
		{
			EndlessTest();
			return 0;
		}
		else if (arg == "-s")		// run separate process test
		{
			SeparateProcessTest();
			return 0;
		}
		else if (arg == "-w")
		{
			std::cout << "Send OutputDebugStringA 'WithoutNewLine'\n";
			OutputDebugStringA("WithoutNewLine ");
			return 0;
		}
		else if (arg == "-n")
		{
			std::cout << "Send OutputDebugStringA 'WithNewLine\\n'\n";
			OutputDebugStringA("WithNewLine\n");
			return 0;
		}
		else if (arg == "-e")
		{
			std::cout << "Send empty OutputDebugStringA message\n";
			OutputDebugStringA("");			//empty message
			return 0;
		}
		else if (arg == "-4")
		{
			std::cout << "Send 2x OutputDebugStringA 'WithNewLine\\n'\n";
			OutputDebugStringA("WithNewLine\n");
			OutputDebugStringA("WithNewLine\n");
			return 0;
		}
		else if (arg == "-5")
		{
			std::cout << "Send OutputDebugStringA '1\\n2\\n3\\n'\n";
			OutputDebugStringA("1\n2\n3\n");
			return 0;
		}
		else if (arg == "-6")
		{
			std::cout << "Send OutputDebugStringA '1 ' '2 ' '3\\n' in separate messages\n";
			OutputDebugStringA("1 ");
			OutputDebugStringA("2 ");
			OutputDebugStringA("3\n");
			return 0;
		}
		else if (arg == "-7")
		{
			DbgMsgTest();
			return 0;
		}
		else if (arg == "-8")
		{
			if (i == lastArgc)
			{
				PrintUsage();
				return -1;
			}
			int n = atoi(argv[i + 1]);
			DbgMsgSrc(n);
			return 0;
		}
		else if (arg == "-9")
		{
			DbgMsgClearTest();
			return 0;
		}
		else if (arg == "-A")
		{
			CoutCerrTest();
			return 0;
		}
		else
		{
			Output(arg);
			return 0;
		}
	}
	PrintUsage();
	return 0;
}
