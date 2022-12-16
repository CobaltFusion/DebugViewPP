#pragma once

#define DECLARE_WND_SUPERCLASS_WORKAROUND(WndClassName, OrigWndClassName)    \
	static ATL::CWndClassInfo& GetWndClassInfo()                             \
	{                                                                        \
		static ATL::CWndClassInfo wc =                                       \
			{                                                                \
				{sizeof(WNDCLASSEX), 0, this->StartWindowProc,               \
					0, 0, NULL, NULL, NULL, NULL, NULL, WndClassName, NULL}, \
				OrigWndClassName, NULL, NULL, TRUE, 0, _T("")};              \
		return wc;                                                           \
	}

#define DECLARE_FRAME_WND_CLASS_EX_WORKAROUND(WndClassName, uCommonResourceID, style, bkgnd) \
	static WTL::CFrameWndClassInfo& GetWndClassInfo()                                        \
	{                                                                                        \
		static WTL::CFrameWndClassInfo wc =                                                  \
			{                                                                                \
				{sizeof(WNDCLASSEX), style, this->StartWindowProc,                           \
					0, 0, NULL, NULL, NULL, (HBRUSH)(bkgnd + 1), NULL, WndClassName, NULL},  \
				NULL, NULL, IDC_ARROW, TRUE, 0, _T(""), uCommonResourceID};                  \
		return wc;                                                                           \
	}
