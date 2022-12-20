// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <atlbase.h>
#include <atlcom.h>
#include <boost/signals2/signal.hpp>

namespace fusion {
namespace debugviewpp {
class ATL_NO_VTABLE DropTargetSupport
    : public CComObjectRootEx<CComSingleThreadModel>
    , public IDropTarget
{
public:
    BEGIN_COM_MAP(DropTargetSupport)
        COM_INTERFACE_ENTRY(IDropTarget)
    END_COM_MAP()

    DropTargetSupport();
    virtual ~DropTargetSupport() = default;
    void Register(HWND hwnd);
    void Unregister();

    STDMETHOD(DragEnter)(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    STDMETHOD(DragLeave)() override;
    STDMETHOD(Drop)(IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;

    using DroppedSignal = boost::signals2::signal<void(const std::wstring&)>;
    boost::signals2::connection SubscribeToDropped(DroppedSignal::slot_type slot);

private:
    HWND m_hwnd = nullptr;
    DroppedSignal m_onDropped;
};

} // namespace debugviewpp
} // namespace fusion
