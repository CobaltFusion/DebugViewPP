// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <streambuf>
#include <vector>

namespace fusion {

template <class Elem, class Tr = std::char_traits<Elem>, class Alloc = std::allocator<Elem>>
class basic_handlebuf : public std::basic_streambuf<Elem, Tr>
{
public:
	using _int_type = typename std::basic_streambuf<Elem, Tr>::int_type;

	explicit basic_handlebuf(HANDLE handle, std::size_t buff_sz = 256, std::size_t put_back = 8) :
		m_handle(handle),
		m_put_back(std::max<std::size_t>(put_back, 1)),
		m_readBuffer(std::max(buff_sz, m_put_back) + m_put_back)
	{
		Elem* end = &m_readBuffer.front() + m_readBuffer.size();
		this->setg(end, end, end);
	}

protected:
	int sync() override
	{
		if (!m_writeBuffer.empty())
		{
			DWORD written;
			if (!WriteFile(m_handle, m_writeBuffer.data(), static_cast<DWORD>(m_writeBuffer.size()), &written, nullptr))
				return std::basic_streambuf<Elem, Tr>::traits_type::eof();

			m_writeBuffer.clear();
		}
		return 0;
	}

	_int_type overflow(_int_type c) override
	{
		if (c == std::basic_streambuf<Elem, Tr>::traits_type::eof())
			return c;

		m_writeBuffer.push_back(std::basic_streambuf<Elem, Tr>::traits_type::to_char_type(c));
		if (c == '\n')
			sync();
		return c;
	}

	_int_type underflow() override
	{
		if (this->gptr() < this->egptr()) // buffer not exhausted
			return std::basic_streambuf<Elem, Tr>::traits_type::to_int_type(*this->gptr());

		Elem* base = &m_readBuffer.front();
		Elem* start = base;

		if (this->eback() == base) // true when this isn't the first fill
		{
			// Make arrangements for putback characters
			std::memmove(base, this->egptr() - m_put_back, m_put_back);
			start += m_put_back;
		}

		// start is now the start of the buffer, proper.
		// Read from m_handle in to the provided buffer
		DWORD read;
		if (!ReadFile(m_handle, start, static_cast<DWORD>((m_readBuffer.size() - (start - base)) * sizeof(Elem)), &read, nullptr) || read == 0)
			return std::basic_streambuf<Elem, Tr>::traits_type::eof();

		// Set buffer pointers
		this->setg(base, start, start + read / sizeof(Elem));

		return std::basic_streambuf<Elem, Tr>::traits_type::to_int_type(*this->gptr());
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
	explicit basic_handlestream(HANDLE handle) :
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
