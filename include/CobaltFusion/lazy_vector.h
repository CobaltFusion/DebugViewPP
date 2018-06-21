// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#ifndef LAZY_VECTOR_H
#define LAZY_VECTOR_H

#pragma once

#include <vector>

template <typename T>
class lazy_vector
{
private:
	using vec = std::vector<T>;

public:
	typename vec::iterator begin()
	{
		return m_vector.begin();
	}

	typename vec::iterator end()
	{
		return m_vector.begin() + m_size;
	}

	T& operator[](int index)
	{
		return m_vector[index];
	}

	void push_back(const T& t)
	{
		if (m_size < m_vector.size())
		{
			m_vector[m_size] = t;
			m_size++;
		}
		else
		{
			m_vector.push_back(t);
			m_size = m_vector.size();
		}
	}

	template <typename... Args>
	void emplace_back(Args&&... args)
	{
		if (m_size < m_vector.size())
		{
			m_vector[m_size] = T(std::forward<Args>(args)...); // unfortunate temporary :( not sure what else to do here
			m_size++;
		}
		else
		{
			m_vector.emplace_back(std::forward<Args>(args)...);
			m_size = m_vector.size();
		}
	}

	void clear()
	{
		m_size = 0;
	}

	void pop_back()
	{
		m_size--;
	}

	void resize(size_t size)
	{
		if (size > m_size)
		{
			m_vector.resize(size);
		}
		m_size = size;
	}

	void reserve(size_t size)
	{
		m_vector.reserve(size);
	}

	size_t size() const
	{
		return m_size;
	}

	void shrink_to_fit()
	{
		m_vector.resize(m_size);
		m_vector.shrink_to_fit();
	}

private:
	size_t m_size = 0;
	std::vector<T> m_vector;
};

#endif // #ifndef LAZY_VECTOR_H
