// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <map>
#include <psapi.h>

namespace fusion {
namespace debugviewpp {

struct InternalProcessProperties
{
	InternalProcessProperties();
	InternalProcessProperties(DWORD pid, const std::string& name, COLORREF color);

	DWORD pid;			// system processId
	std::string name;
	COLORREF color;
};

struct ProcessProperties
{
	explicit ProcessProperties(const InternalProcessProperties& iprops);

	DWORD uid;			// unique id
	DWORD pid;			// system processId
	std::string name;
	COLORREF color;
};

class ProcessInfo
{
public:
	ProcessInfo();
	void Clear();
	static size_t GetPrivateBytes();
	static std::wstring GetProcessName(HANDLE handle);
	static std::wstring GetProcessNameByPid(DWORD processId);

	DWORD GetUid(DWORD processId, const std::string& processName);
	ProcessProperties GetProcessProperties(DWORD processId, const std::string& processName);
	ProcessProperties GetProcessProperties(DWORD uid) const;

private:
	std::map<DWORD, InternalProcessProperties> m_processProperties;
	DWORD m_unqiueId;
};

} // namespace debugviewpp 
} // namespace fusion
