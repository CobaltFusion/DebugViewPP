// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "atlbase.h"
#include "atlcom.h"

namespace fusion {
namespace Win32 {

class ComInitialization
{
public:
	explicit ComInitialization();
	~ComInitialization();
};

template <typename T>
class ComObjectPtr
{
public:
	ComObjectPtr<T>() : ptr(nullptr)
	{
	}

	ComObjectPtr<T>(const ComObjectPtr<T>& p) : ptr(p)
	{
		ptr->AddRef();
	}

	const ComObjectPtr<T>& operator=(const ComObjectPtr<T>& p)
	{
		if (ptr != nullptr) ptr->Release();
		ptr = p.ptr;
		if (ptr != nullptr) ptr->AddRef();
		return *this;
	}

	const ComObjectPtr<T>& operator=(const ATL::CComObject<T>* p)
	{
		if (ptr != nullptr) ptr->Release();
		ptr = p;
		if (ptr != nullptr) ptr->AddRef();
		return *this;
	}

	ComObjectPtr(ATL::CComObject<T>* p) : ptr(p)
	{
		ptr->AddRef();
	}

	~ComObjectPtr()
	{
		ptr->Release();
	}

	T* operator->() const
	{
		return ptr;
	}

private:
	ATL::CComObject<T> * ptr;
};

template <typename Itf, typename T>
ATL::CComPtr<Itf> GetInterface(ComObjectPtr<T> ptr)
{
	ATL::CComPtr<Itf> pItf;
	ptr->QueryInterface(&pItf);
	return pItf;
}

template <typename Itf, typename T>
ATL::CComPtr<Itf> GetInterface(T * ptr)
{
	ATL::CComPtr<Itf> pItf;
	ptr->QueryInterface(&pItf);
	return pItf;
}

template <typename T>
ComObjectPtr<T> CreateComObject()
{
	ATL::CComObject<T> * ptr;
	CComObject<T>::CreateInstance(&ptr);
	return ComObjectPtr<T>(ptr);
}

} // namespace Win32
} // namespace fusion
