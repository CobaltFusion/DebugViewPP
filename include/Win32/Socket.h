// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <memory>
#include <boost/utility.hpp>
#include "winsock.h"

namespace fusion {
namespace Win32 {

class WinsockInitialization : boost::noncopyable
{
public:
	explicit WinsockInitialization(int major = 2, int minor = 2);
	~WinsockInitialization();
};

void WSAThrowLastError(const std::string& what);

struct SocketDeleter
{
	typedef SOCKET pointer;

	void operator()(pointer p) const;
};

typedef std::unique_ptr<void, SocketDeleter> Socket;

} // namespace Win32	
} // namespace fusion
