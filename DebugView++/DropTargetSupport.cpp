// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

// #include "stdafx.h"
#include "DropTargetSupport.h"
#include "CobaltFusion/Str.h"

namespace fusion {
namespace debugviewpp {

/* class inspired by:
 * http://www.catch22.net/tuts/drag-and-drop-introduction
 * and also:
 * https://www.codeproject.com/Articles/840/How-to-Implement-Drag-and-Drop-Between-Your-Progra
 */

DropTargetSupport::DropTargetSupport()

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

bool QueryDataObject(IDataObject* pDataObject, CLIPFORMAT format)
{
    FORMATETC fmtetc = {format, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    // does the data object support CF_TEXT using a HGLOBAL?
    return pDataObject->QueryGetData(&fmtetc) == S_OK;
}

STDMETHODIMP DropTargetSupport::DragEnter(IDataObject* pDataObject, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
    if (QueryDataObject(pDataObject, CF_TEXT) || QueryDataObject(pDataObject, CF_HDROP))
    {
        *pdwEffect = DROPEFFECT_COPY;
    }
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

std::string GetCF_TEXT(IDataObject* pDataObject)
{
    std::string result;
    // construct a FORMATETC object
    FORMATETC fmtetc = {CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmed;

    if (pDataObject->GetData(&fmtetc, &stgmed) == S_OK)
    {
        // we asked for the data as a HGLOBAL, so access it appropriately
        PVOID data = GlobalLock(stgmed.hGlobal);
        result = std::string(static_cast<char*>(data));
        GlobalUnlock(stgmed.hGlobal);

        // release the data using the COM API
        ReleaseStgMedium(&stgmed);
    }
    return result;
}

std::wstring GetCF_HDROP(IDataObject* pDataObject)
{
    std::wstring result;
    // construct a FORMATETC object
    FORMATETC fmtetc = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmed;

    if (pDataObject->GetData(&fmtetc, &stgmed) == S_OK)
    {
        // we asked for the data as a HGLOBAL, so access it appropriately
        auto hDropInfo = static_cast<HDROP>(GlobalLock(stgmed.hGlobal));

        if (DragQueryFile(hDropInfo, 0xFFFFFFFF, nullptr, 0) == 1)
        {
            std::vector<wchar_t> filename(DragQueryFile(hDropInfo, 0, nullptr, 0) + 1);
            if (DragQueryFile(hDropInfo, 0, filename.data(), static_cast<UINT>(filename.size())))
            {
                result = std::wstring(filename.data());
            }
        }
        GlobalUnlock(stgmed.hGlobal);

        // release the data using the COM API
        ReleaseStgMedium(&stgmed);
    }
    return result;
}

STDMETHODIMP DropTargetSupport::Drop(IDataObject* pDataObject, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
    if (QueryDataObject(pDataObject, CF_TEXT))
    {
        *pdwEffect = DROPEFFECT_COPY;
        m_onDropped(WStr(GetCF_TEXT(pDataObject)));
        return S_OK;
    }

    if (QueryDataObject(pDataObject, CF_HDROP))
    {
        *pdwEffect = DROPEFFECT_COPY;
        m_onDropped(GetCF_HDROP(pDataObject));
    }

    return S_OK;
}

boost::signals2::connection DropTargetSupport::SubscribeToDropped(DroppedSignal::slot_type slot)
{
    return m_onDropped.connect(slot);
}

} // namespace debugviewpp
} // namespace fusion
