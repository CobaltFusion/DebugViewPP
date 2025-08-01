// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "PipeReader.h"
#include "PolledLogSource.h"

#include "Win32/Win32Lib.h"

#include "Debugview_kernel_client.h"

namespace fusion {
namespace debugviewpp {

std::string GetDebugviewDriverLocation();

class ILineBuffer;

class KernelReader : public PolledLogSource
{
public:
    KernelReader(Timer& timer, ILineBuffer& linebuffer);
    ~KernelReader() override;
    void Abort() override;
    bool AtEnd() const override;

    void SetVerbose(bool value);
    void SetPassThrough(bool value);

private:
    void Poll() override;

    void StartListening();
    void StopListening();
    void SetKernelMessagesDriverFeature(DWORD);

    Win32::Handle m_handle;
    PLOG_ITEM m_pBuf;
};

} // namespace debugviewpp
} // namespace fusion
