// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <vector>

namespace fusion {

class Process
{
public:
	Process(const std::wstring& pathName, const std::vector<std::wstring>& args);
	Process(const std::wstring& pathName, const std::wstring& args);

	std::wstring GetName() const;
	HANDLE GetStdIn() const;
	HANDLE GetStdOut() const;
	HANDLE GetStdErr() const;
	HANDLE GetProcessHandle() const;
	HANDLE GetThreadHandle() const;
	unsigned GetProcessId() const;
	unsigned GetThreadId() const;

	bool IsRunning() const;
	void Wait() const;

private:
	void Run(const std::wstring& pathName, const std::wstring& args);

	std::wstring m_name;
	CHandle m_stdIn;
	CHandle m_stdOut;
	CHandle m_stdErr;
	CHandle m_hProcess;
	CHandle m_hThread;
	unsigned m_processId;
	unsigned m_threadId;
};

} // namespace fusion
