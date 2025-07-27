// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CobaltFusion/Str.h"
#include "DebugViewppLib/PolledLogSource.h"
#include "DebugViewppLib/KernelReader.h"
#include "DebugViewppLib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

KernelReader::KernelReader(Timer& timer, ILineBuffer& linebuffer) :
    PolledLogSource(timer, SourceType::Pipe, linebuffer, 1)
{
    SetDescription(L"Kernel Message Reader");
    AddMessage(0, "kernel", "Started capturing kernel messages");
    Signal();
    StartThread();
}

KernelReader::~KernelReader() = default;

void KernelReader::Abort()
{
    AddMessage(0, "kernel", "<kernel message reader aborted>");
    Signal();
    PolledLogSource::Abort();
}

bool KernelReader::AtEnd() const
{
    return false;
}

void KernelReader::Poll()
{
    AddMessage(0, "kernel", "Test message...");
}

} // namespace debugviewpp
} // namespace fusion
