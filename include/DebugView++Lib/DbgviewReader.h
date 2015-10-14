// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/asio.hpp>
#include "PassiveLogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

template <typename T, class S> 
T Read(S& is)
{
	T t = T();
	is.read(reinterpret_cast<char*>(&t), sizeof(t));
	return t;
}

template <typename T, class S> 
void Write(S& is, T t)
{
	is.write(reinterpret_cast<const char*>(&t), sizeof(t));
}

class DbgviewReader : public PassiveLogSource
{
public:
	DbgviewReader(Timer& timer, ILineBuffer& linebuffer, const std::string& hostname);
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
