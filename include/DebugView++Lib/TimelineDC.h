// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "windows.h"
#include <atlbase.h>
#include "atlapp.h"
#include "atlgdi.h"
#include <string>
#include <vector>

namespace fusion {
namespace graphics {

class Window
{
public:
	Window(HINSTANCE hInstance, WNDPROC messageHandler, const std::wstring& uniqueClassName, const std::wstring& title, int width, int height);
	void Show(int nCmdShow);

private:
	WNDCLASSEX wc;
	HWND hwnd;
};

class MessageLoop
{
public:
	int run();
private:
	MSG msg;
};

// see http://www.informit.com/articles/article.aspx?p=328647&seqNum=2

static const int s_drawTimelineMax = 150;

using ManagedCDC = CDCT<true>;

class TimelineDC : public ManagedCDC
{
	using ManagedCDC::ManagedCDC;
public:
	RECT GetClientArea();
	void DrawTextOut(const std::wstring& str, int x, int y);
	void DrawPolygon(const std::vector<POINT>& points);
	void DrawTimeline(const std::wstring& name, int x, int y, int width, COLORREF color);
	void DrawFlag(const std::wstring& /* tooltip */, int x, int y);
	void DrawSolidFlag(const std::wstring& /* tooltip */, int x, int y);
	void DrawSolidFlag(const std::wstring& /* tooltip */, int x, int y, COLORREF border, COLORREF fill);
	void DrawFlag(const std::wstring& /* tooltip */, int x, int y, COLORREF color, bool solid);
};


} // namespace graphics
} // namespace fusion
