// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "windows.h"
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

class DeviceContext
{
public:
	explicit DeviceContext(HDC hDC);
	void MoveTo(int x, int y);
	void LineTo(int x, int y);
	void DrawTextOut(const std::wstring& str, int x, int y);
	void Rectangle(int x, int y, int width, int height);
	void DrawPolygon(const std::vector<POINT>& points);

protected:
	HDC hDC;
};

// see http://www.informit.com/articles/article.aspx?p=328647&seqNum=2


class DeviceContextEx : public DeviceContext
{
	using DeviceContext::DeviceContext;
public:

	void DrawTimeline(const std::wstring& name, int x, int y, int width, COLORREF color);
	void DrawFlag(const std::wstring& /* tooltip */, int x, int y);
	void DrawSolidFlag(const std::wstring& /* tooltip */, int x, int y);
	void DrawSolidFlag(const std::wstring& /* tooltip */, int x, int y, COLORREF border, COLORREF fill);
	void DrawFlag(const std::wstring& /* tooltip */, int x, int y, COLORREF color, bool solid);
};


} // namespace graphics
} // namespace fusion
