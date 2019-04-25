// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <atlcom.h>
#include <boost/signals2/signal.hpp>

namespace fusion {
namespace debugviewpp {

class ATL_NO_VTABLE DropTargetSupport : public CComObjectRootEx<CComSingleThreadModel>, public IDropTarget
{
public:
	BEGIN_COM_MAP(DropTargetSupport)
		COM_INTERFACE_ENTRY(IDropTarget)
	END_COM_MAP()

	DropTargetSupport();
	virtual ~DropTargetSupport() {}
	void Register(HWND hwnd);
	void Unregister();

	STDMETHOD(DragEnter)(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	STDMETHOD(DragLeave)();
	STDMETHOD(Drop)(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

	typedef boost::signals2::signal<void(const std::wstring& uri)> DroppedSignal;
	boost::signals2::connection SubscribeToDropped(DroppedSignal::slot_type slot);

private:
	FORMATETC m_fe;
	HWND m_hwnd;
	DroppedSignal m_onDropped;
};

} // namespace debugviewpp
} // namespace fusion
