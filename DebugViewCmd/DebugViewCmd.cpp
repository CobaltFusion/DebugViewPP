// DebugViewCmd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace fusion;
using namespace fusion::debugviewpp;

int _tmain(int argc, _TCHAR* argv[])
{
	std::vector<std::unique_ptr<LogSource>> sources;
	
	LineBuffer linebuffer(2000);
	sources.push_back(make_unique<DBWinReader>(linebuffer, false));
	if (HasGlobalDBWinReaderRights())
	{
		sources.push_back(make_unique<DBWinReader>(linebuffer, true));
	}

	auto pred = [](const Line& a, const Line& b) { return a.time < b.time; };
	while (true)
	{
		Lines lines;
		for (auto it = sources.begin(); it != sources.end(); )
		{
			Lines pipeLines((*it)->GetLines());
			Lines lines2;
			lines2.reserve(lines.size() + pipeLines.size());
			std::merge(lines.begin(), lines.end(), pipeLines.begin(), pipeLines.end(), std::back_inserter(lines2), pred);
			lines.swap(lines2);

			if ((*it)->AtEnd())
				it = sources.erase(it);
			else
				++it;
		}

		for (auto i=lines.begin(); i != lines.end(); ++i)
		{
			printf("%i\t%s\t%s\n", i->pid, i->processName.c_str(), i->message.c_str());
		}
		Sleep(100);
	}
}
