// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "psapi.h"

namespace fusion {

struct InternalProcessProperties
{
	explicit InternalProcessProperties() :
		pid(0)
	{
	}

	explicit InternalProcessProperties(DWORD pid_, const std::string& name_) :
		pid(pid_), name(name_)
	{
	}

	DWORD pid;			// system processId
	std::string name;
};

struct ProcessProperties
{
	ProcessProperties(const InternalProcessProperties& iprops) :
		uid(0), pid(iprops.pid), name(iprops.name)
	{
	}
	DWORD uid;			// unique id
	DWORD pid;			// system processId
	std::string name;
};

class ProcessInfo
{
public:
	ProcessInfo();
	void Reset();
	static size_t GetPrivateBytes();
	static std::wstring GetProcessName(HANDLE handle);
	static std::wstring GetProcessName(DWORD processId);

	DWORD GetUid(DWORD processId, const std::string& processName);
	ProcessProperties GetProcessProperties(DWORD processId, const std::string& processName);
	ProcessProperties GetProcessProperties(DWORD uid) const;

private:
	typedef std::map<DWORD, InternalProcessProperties> ProcessMap;
	ProcessMap m_processProperties;
	DWORD m_unqiueId;
};

} // namespace fusion
