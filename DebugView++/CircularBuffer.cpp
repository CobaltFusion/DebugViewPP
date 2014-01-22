// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "CircularBuffer.h"

namespace fusion {
namespace debugviewpp {

CircularBuffer::CircularBuffer(size_t size) :
	m_pBegin(new char[size]),
	m_pEnd(m_pBegin + size),
	m_pRead(m_pBegin),
	m_pWrite(m_pBegin)
{
}

void CircularBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const char* message)
{

}

Lines CircularBuffer::GetLines()
{
	return Lines();
}

CircularBuffer::~CircularBuffer()
{
	delete[] m_pBegin;
}

} // namespace debugviewpp
} // namespace fusion
