// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "PipeReader.h"
#include "PassiveLogSource.h"
#include "Process.h"
#include <boost/asio.hpp> 

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

template<typename T, class S> 
T Read(S& is)
{
	T t = T();
	is.read((char*) &t, sizeof(T));
	return t;
}

class DbgviewReader : public PassiveLogSource
{
public:
	explicit DbgviewReader(Timer& timer, ILineBuffer& linebuffer, const std::string& hostname);
	virtual ~DbgviewReader();

	virtual void Abort();

private:
	void Loop();
	std::string m_hostname;
	boost::asio::ip::tcp::iostream m_iostream;

	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
