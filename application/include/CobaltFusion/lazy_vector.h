// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef LAZY_VECTOR_H
#define LAZY_VECTOR_H

#pragma once

#include <vector>

namespace fusion {

template <typename T>
class lazy_vector
{
private:
    using lazy_vector_t = std::vector<T>;

public:
    typename lazy_vector_t::iterator begin()
    {
        return m_vector.begin();
    }

    typename lazy_vector_t::iterator end()
    {
        return m_vector.begin() + m_size;
    }

    typename lazy_vector_t::const_iterator begin() const
    {
        return m_vector.begin();
    }

    typename lazy_vector_t::const_iterator end() const
    {
        return m_vector.begin() + m_size;
    }

    T& operator[](size_t index)
    {
        return m_vector[index];
    }

    T& at(size_t index)
    {
        if (index < m_size)
            return m_vector[index];
        throw std::runtime_error("invalid lazy_vector<T> subscript");
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

    bool empty() const
    {
        return m_size == 0;
    }

    void shrink_to_fit()
    {
        m_vector.resize(m_size);
        m_vector.shrink_to_fit();
    }

private:
    size_t m_size = 0;
    lazy_vector_t m_vector;
};

} // namespace fusion

#endif // #ifndef LAZY_VECTOR_H
