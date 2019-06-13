// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "noncopyable.h"

namespace fusion {

template <typename F>
class scope_guard : fusion::noncopyable
{
public: 
	explicit scope_guard(const F& x) :
		m_action(x),
		m_released(false)
	{
	}

	scope_guard(scope_guard&& rhs) : 
		m_action(std::move(rhs.m_action)),
		m_released(rhs.m_released)
	{
		rhs.m_released = true;
	}

	void release()
	{
		m_released = true;
	}

	~scope_guard()
	{
		if (m_released)
			return;

		// Catch any exceptions and continue during guard clean up
		try
		{
			m_action();
		}
		catch (...)
		{
			assert(false && "exception in guard clean up");
		}
	}

private:
	F m_action;
	bool m_released;
};

template <typename F>
scope_guard<F> make_guard(F f)
{
	return scope_guard<F>(f);
}

} // namespace fusion
