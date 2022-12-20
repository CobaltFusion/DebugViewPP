// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "LineBuffer.h"

namespace fusion {
namespace debugviewpp {

class VectorLineBuffer : public ILineBuffer
{
public:
    explicit VectorLineBuffer(size_t size);

    void Add(double time, FILETIME systemTime, HANDLE handle, const std::string& message, const LogSource* pSource) override;
    void Add(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message, const LogSource* pSource) override;
    [[nodiscard]] Lines GetLines() override;
    [[nodiscard]] bool Empty() const override;

private:
    std::mutex m_linesMutex;
    Lines m_buffer;
    Lines m_backingBuffer;
};

using LineBuffer = VectorLineBuffer;

} // namespace debugviewpp
} // namespace fusion
