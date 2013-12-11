#include <iostream>
#include <Windows.h>
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

int main(int argc, char* argv[])
{
	int n = 0;
	if (argc == 2 && (n = atoi(argv[1])) > 0)
		DbgMsgSrc(n);

	std::cout << "Syntax: DbgMsgSrc <frequency>\n";
}
