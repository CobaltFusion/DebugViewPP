// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include "DebugviewppLib/LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class Loopback : public LogSource
{
public:
    Loopback(Timer& timer, ILineBuffer& lineBuffer);
    virtual ~Loopback();

    HANDLE GetHandle() const override { throw std::exception("should never be called"); }
    void Notify() override {}

    bool GetAutoNewLine() const override;
    void PreProcess(Line& line) const override;
};

} // namespace debugviewpp
} // namespace fusion
