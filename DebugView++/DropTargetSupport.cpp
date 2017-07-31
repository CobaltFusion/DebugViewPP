// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DropTargetSupport.h"

namespace fusion {
namespace debugviewpp {

DropTargetSupport::DropTargetSupport()
    : m_fe({0})
    , m_hwnd(nullptr)
{
}

void DropTargetSupport::Register(HWND hwnd)
{
	m_hwnd = hwnd;
	RegisterDragDrop(hwnd, this);
}

void DropTargetSupport::Unregister()
{
	RevokeDragDrop(m_hwnd);
}

STDMETHODIMP DropTargetSupport::DragEnter(IDataObject* pDataObject, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{

    *pdwEffect = DROPEFFECT_SCROLL;
	CComPtr<IEnumFORMATETC> pEnum;
	pDataObject->EnumFormatEtc(DATADIR_GET, &pEnum);
	while (pEnum->Next(1, &m_fe, nullptr) == NO_ERROR)
	{
		if (m_fe.cfFormat == CF_TEXT)
		{
			*pdwEffect = DROPEFFECT_COPY;
			break;
		}
	}
    *pdwEffect = DROPEFFECT_COPY;

	return S_OK;
}

STDMETHODIMP DropTargetSupport::DragOver(DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
	*pdwEffect = DROPEFFECT_COPY;
	return S_OK;
}

STDMETHODIMP DropTargetSupport::DragLeave()
{
	return S_OK;
}

STDMETHODIMP DropTargetSupport::Drop(IDataObject* pDataObject, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
    auto stg = STGMEDIUM();
	pDataObject->GetData(&m_fe, &stg);
	auto lpData = static_cast<LPCTSTR>(GlobalLock(stg.hGlobal));

	//m_view.SetWindowText(lpData);
    OutputDebugString(lpData);
	GlobalUnlock(stg.hGlobal);

	*pdwEffect = DROPEFFECT_COPY;
	return S_OK;
}

} // namespace debugviewpp
} // namespace fusion