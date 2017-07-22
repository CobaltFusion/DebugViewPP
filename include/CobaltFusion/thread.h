// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <thread>
#include <cassert>

namespace fusion {

class thread : private std::thread
{
public:
    explicit thread(std::function<void()> fn) : std::thread(fn)
	{
	}

	~thread()
	{
		assert(!joinable() && "Always join the thread before releasing it");
	}

	void join()
	{
		if (joinable()) std::thread::join();
	}

	void detach()
	{
		assert(!joinable() && "Cannot detach an unjoinable thread");
		std::thread::detach();
	}

	id get_id() const
	{
		return std::thread::get_id();
	}
};

} // namespace fusion
