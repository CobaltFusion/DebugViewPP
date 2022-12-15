// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <stdexcept>
#include "DebugView++Lib/FilterType.h"

namespace fusion {
namespace debugviewpp {

int FilterTypeToInt(FilterType::type value)
{
#define FILTER_TYPE(f, id) \
    case FilterType::f: return id;
    switch (value)
    {
        FILTER_TYPES()
    default: assert(!"Unexpected FilterType"); break;
    }
#undef FILTER_TYPE

    throw std::invalid_argument("bad FilterType!");
}

FilterType::type IntToFilterType(int value)
{
#define FILTER_TYPE(f, id) \
    case id: return FilterType::f;
    switch (value)
    {
        FILTER_TYPES()
    default: assert(!"Unexpected FilterType"); break;
    }
#undef FILTER_TYPE

    throw std::invalid_argument("bad FilterType!");
}

std::string FilterTypeToString(FilterType::type value)
{
#define FILTER_TYPE(f, id) \
    case FilterType::f: return #f;
    switch (value)
    {
        FILTER_TYPES()
    default: assert(!"Unexpected FilterType"); break;
    }
#undef FILTER_TYPE

    throw std::invalid_argument("bad FilterType!");
}

const wchar_t* EnumToWCharPtr(FilterType::type value)
{
#define FILTER_TYPE(f, id) \
    case FilterType::f: return L## #f;
    switch (value)
    {
        FILTER_TYPES()
    default: assert(!"Unexpected FilterType"); break;
    }
#undef FILTER_TYPE

    throw std::invalid_argument("bad FilterType!");
}

FilterType::type StringToFilterType(const std::string& s)
{
#define FILTER_TYPE(f, id) \
    if (s == #f)           \
        return FilterType::f;
    FILTER_TYPES()
#undef FILTER_TYPE

    throw std::invalid_argument("bad FilterType!");
}

} // namespace debugviewpp
} // namespace fusion
