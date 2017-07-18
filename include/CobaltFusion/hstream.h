// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <streambuf>
#include <vector>

namespace fusion {

template <class Elem, class Tr = std::char_traits<Elem>, class Alloc = std::allocator<Elem> >
class basic_handlebuf : public std::basic_streambuf<Elem, Tr>
{
public:
	basic_handlebuf(HANDLE handle, std::size_t buff_sz = 256, std::size_t put_back = 8) :
		m_handle(handle),
		m_put_back(std::max<std::size_t>(put_back, 1)),
		m_readBuffer(std::max(buff_sz, m_put_back) + m_put_back)
	{
		Elem* end = &m_readBuffer.front() + m_readBuffer.size();
		setg(end, end, end);
	}

protected:
    virtual int sync() override
	{
		if (!m_writeBuffer.empty())
		{
			DWORD written;
			if (!WriteFile(m_handle, m_writeBuffer.data(), static_cast<DWORD>(m_writeBuffer.size()), &written, nullptr))
				return traits_type::eof();

			m_writeBuffer.clear();
		}
		return 0;
	}

    virtual int_type overflow(int_type c) override
	{
		if (c == traits_type::eof())
			return c;

		m_writeBuffer.push_back(traits_type::to_char_type(c));
		if (c == '\n')
			sync();
		return c;
	}

    virtual int_type underflow() override
	{
		if (gptr() < egptr()) // buffer not exhausted
			return traits_type::to_int_type(*gptr());

		Elem* base = &m_readBuffer.front();
		Elem* start = base;

		if (eback() == base) // true when this isn't the first fill
		{
			// Make arrangements for putback characters
			std::memmove(base, egptr() - m_put_back, m_put_back);
			start += m_put_back;
		}

		// start is now the start of the buffer, proper.
		// Read from m_handle in to the provided buffer
		DWORD read;
		if (!ReadFile(m_handle, start, static_cast<DWORD>((m_readBuffer.size() - (start - base))*sizeof(Elem)), &read, nullptr) || read == 0)
			return traits_type::eof();

		// Set buffer pointers
		setg(base, start, start + read/sizeof(Elem));

		return traits_type::to_int_type(*gptr());
	}

private:
	HANDLE m_handle;
	const std::size_t m_put_back;
	std::vector<Elem> m_readBuffer;
	std::vector<Elem> m_writeBuffer;
};

template <class Elem, class Tr = std::char_traits<Elem>>
class basic_handlestream : public std::basic_iostream<Elem, Tr>
{
public:
	basic_handlestream(HANDLE handle) :
		std::basic_iostream<Elem, Tr>(&m_buf),
		m_buf(handle)
	{
	}

private:
	basic_handlebuf<Elem, Tr> m_buf;
};

typedef basic_handlestream<char> hstream;
typedef basic_handlestream<wchar_t> whstream;

} // namespace fusion
