#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#include <stdio.h>
#pragma comment(lib, "psapi.lib")

#include <iostream>
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
				cdbg << "Een, twee, drie, vier, vijf, zes, zeven, acht, negen, tien, elf, twaalf, dertien, veertien, vijftien, zestien, zeventien, achttien, negentien, twintig\n";
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
}


#include "windows.h"
#include <stdio.h>
#include <string>

#include <iostream>
using namespace std;
#include <sys/timeb.h>
#include <sstream>
#include <fstream>
#include <vector>

int getMilliCount(){
	timeb tb;
	ftime(&tb);
	int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return nCount;
}

void testLongString()
{
	int length = 15000;
	std::ostringstream ss;
	ss << "1234567890ABCDEF-%s {0} \t\r\n";
	ss << length;
	std::string test = ss.str();
	for (size_t i=0;i<length-test.size()-1; i++)
	{
		ss << "X";
	}
	ss << "\n";
	test = ss.str();	
	
	OutputDebugStringA(test.c_str());
}

void Output(std::wstring filename)
{
	printf("Buggazer tester, PID: %d\n", GetCurrentProcessId());
	OutputDebugStringA("Output Titan crash log\n");
	std::fstream fs;
	fs.open(filename, std::fstream::in);

	if (!fs.good())
	{
		printf("'%S' not found!\n", filename.c_str());
		return;
	}

	printf("read\n");
	std::vector<std::string> lines;
	while (!fs.eof())
	{
		std::string line;
		getline(fs, line);
		lines.push_back(line);
	}
	fs.close();

	printf("writing...%d lines\n", lines.size());

	int i = 0;
	long t1 = getMilliCount();

	for (auto s= lines.begin(); s != lines.end(); s++)
	{
		++i;

		//auto t = s + "\n";
		OutputDebugStringA(s->c_str());
		//Sleep(50);
	}
	long t2 = getMilliCount();

	printf("OutputDebugStringA %d lines, took: %u ms\n", i, t2 - t1);
}

void EndlessTest()
{
	int length = 40;		//260
	std::ostringstream ss;
	ss << "123456:jan-7890_ABC__DEF-te m_test";
	ss << length;
	std::string test = ss.str();
	for (size_t i = 0; i<length - test.size() - 1; i++)
	{
		ss << "X";
	}
	ss << "\n";
	test = ss.str();

	//testLongString();

	while (1)
	{
		long t1 = getMilliCount();

		for (int a = 0; a < 5; a++)
		{
			OutputDebugStringA("    ## before me ##\n");

			// write exactly 2MB to hte debug buffer;
			for (int i = 0; i < 9; i++)
			{
				OutputDebugStringA(test.c_str());
			}
			long t2 = getMilliCount();

			OutputDebugStringA("    ## tracking me ##\n");
			printf("took: %u ms\n", t2 - t1);
			Sleep(50);
		}
		//OutputDebugStringA("    ----> DBGVIEWCLEAR\n");

	}
}

void SeparateProcessTest()
{
	while (1)
	{
		ShellExecute(0, L"open", L"BugGazerTester.exe", L"-n", NULL, SW_HIDE);
		Sleep(100);
	}
	
}

void PrintUsage()
{
	printf(\
		"Usage:\n" \
		"  -1 read 'titan_crash_debugview_43mb.log' and output it through OutputDebugStringA\n" \
		"  -2 <filename> read <filename> and output it through OutputDebugStringA\n" \
		"  -3 run endless test\n" \
		"  -s run -n repeatedly (10x / second) in separate processes)\n" \
		"  -w Send OutputDebugStringA 'WithoutNewLine'\n" \
		"  -n Send OutputDebugStringA 'WithNewLine\\n'\n" \
		"  -e Send empty OutputDebugStringA message (does not trigger DBwinMutex!)\n" \
		"  -4 Send 2x OutputDebugStringA 'WithNewLine\\n' (process handle cache test)\n" \
		"  -5 Send OutputDebugStringA '1\\n2\\n3\\n'\n" \
		"  -6 Send OutputDebugStringA '1 ' '2 ' '3\\n' in separate messages\n" \
		"\n" \
		);
}


int _tmain(int argc, _TCHAR* argv[])
{
	//OutputDebugStringA("ping");
	//return 0;

	// get un-spoofable executable file name 
	//WCHAR buf[260];
	//GetMappedFileName(GetCurrentProcess(), _tmain, buf, sizeof(buf));
	//printf("%S\n", buf);

	//HANDLE handle1 = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	//HANDLE handle2 = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	//HANDLE handle3 = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	//printf(" %p, %p, %p\n", handle1, handle2, handle3);
	
	int lastArgc = argc - 1;
	for (int i = 1; i < argc; i++) {
		if (_tcscmp(argv[i], L"-1") == 0) 
		{
			Output(L"titan_crash_debugview_43mb.log");
			return 0;
		}
		else if (_tcscmp(argv[i], L"-2") == 0) 
		{
			if (i == lastArgc)
			{
				PrintUsage();
				return -1;
			}
			Output(argv[i + 1]);
			return 0;
		}
		else if (_tcscmp(argv[i], L"-3") == 0)
		{
			EndlessTest();
			return 0;
		}
		else if (_tcscmp(argv[i], L"-s") == 0)		// run separate process test
		{
			SeparateProcessTest();
			return 0;
		}
		else if (_tcscmp(argv[i], L"-w") == 0)
		{
			printf("Send OutputDebugStringA 'WithoutNewLine'\n");
			OutputDebugStringA("WithoutNewLine ");
			return 0;
		}
		else if (_tcscmp(argv[i], L"-n") == 0)
		{
			printf("Send OutputDebugStringA 'WithNewLine\\n'\n");
			OutputDebugStringA("WithNewLine\n");
			return 0;
		}
		else if (_tcscmp(argv[i], L"-e") == 0)
		{
			printf("Send empty OutputDebugStringA message\n");
			OutputDebugStringA("");			//empty message
			return 0;
		}
		else if (_tcscmp(argv[i], L"-4") == 0)
		{
			printf("Send 2x OutputDebugStringA 'WithNewLine\\n'\n");
			OutputDebugStringA("WithNewLine\n");
			OutputDebugStringA("WithNewLine\n");
			return 0;
		}
		else if (_tcscmp(argv[i], L"-5") == 0)
		{
			printf("Send OutputDebugStringA '1\\n2\\n3\\n'\n");
			OutputDebugStringA("1\n2\n3\n");
			return 0;
		}
		else if (_tcscmp(argv[i], L"-6") == 0)
		{
			printf("Send OutputDebugStringA '1 ' '2 ' '3\\n' in separate messages\n");
			OutputDebugStringA("1 ");
			OutputDebugStringA("2 ");
			OutputDebugStringA("3\n");
			return 0;
		}
		else if (_tcscmp(argv[i], L"-7") == 0)
		{
			DbgMsgTest();
			return 0;
		}
		else if (_tcscmp(argv[i], L"-8") == 0) 
		{
			if (i == lastArgc)
			{
				PrintUsage();
				return -1;
			}
			int n = _wtoi(argv[i + 1]);
			DbgMsgSrc(n);
			return 0;
		}
		
	}
	PrintUsage();
	return 0;
}
