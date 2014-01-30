// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

namespace fusion {

class ComInitialization : boost::noncopyable
{
public:
	enum CoInit
	{
		ApartmentThreaded = COINIT_APARTMENTTHREADED,
		Multithreaded = COINIT_MULTITHREADED
	};

	explicit ComInitialization(CoInit init);
	~ComInitialization();
};

WINDOWPLACEMENT GetWindowPlacement(HWND hwnd);
POINT GetMessagePos();
POINT GetCursorPos();

} // namespace fusion
