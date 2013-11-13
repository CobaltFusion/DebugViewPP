#pragma once

#include <queue>
#include <functional>
#include <boost/thread.hpp>

namespace gj {

namespace detail {

template <typename Observer>
class HiddenWindow :
	public ATL::CWindowImpl<HiddenWindow<Observer>, ATL::CWindow, ATL::CNullTraits>
{
public:
	explicit HiddenWindow(Observer& observer) :
		m_pObserver(&observer)
	{
	}

	HWND Create()
	{
		return ATL::CWindowImpl<HiddenWindow<Observer>, ATL::CWindow, ATL::CNullTraits>::Create(HWND_MESSAGE);
	}

	BEGIN_MSG_MAP(CWinHidden)
		MESSAGE_HANDLER(WM_APP, OnMessage)
	END_MSG_MAP()

	void Notify()
	{
		PostMessage(WM_APP);
	}

	LRESULT OnMessage(UINT, WPARAM, LPARAM, BOOL&)
	{
		m_pObserver->OnMessage();
		return 0;
	}

private:
	Observer* m_pObserver;
};

} // namespace detail

class GuiThread
{
public:
	GuiThread() :
		m_wnd(*this)
	{
		m_wnd.Create();
	}

	~GuiThread()
	{
		m_wnd.DestroyWindow();
	}

	void operator()(std::function<void ()> fn)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		bool notify = m_q.empty();
		m_q.emplace(std::move(fn));
		lock.unlock();

		if (notify)
			m_wnd.Notify();
	}

	void OnMessage()
	{
		std::queue<std::function<void ()>> fnq;
		{
			boost::mutex::scoped_lock lock(m_mtx);
			m_q.swap(fnq);
		}

		while (!fnq.empty())
		{
			fnq.front()();
			fnq.pop();
		}
	}

private:
	detail::HiddenWindow<GuiThread> m_wnd;
	boost::mutex m_mtx;
	std::queue<std::function<void ()>> m_q;
};

} // namespace gj
