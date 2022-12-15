// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include "LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class TestSource : public LogSource
{
public:
    TestSource(Timer& timer, ILineBuffer& linebuffer);

    bool AtEnd() const override;
    HANDLE GetHandle() const override;
    void Notify() override;
};

} // namespace debugviewpp
} // namespace fusion
