// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/core/noncopyable.hpp>
#include <atlbase.h>
#include <atlwin.h>
#include "CobaltFusion/Executor.h"
#include <cassert>

namespace fusion {

namespace detail {

template <typename Observer>
class HiddenWindow :
	public ATL::CWindowImpl<HiddenWindow<Observer>, ATL::CWindow, ATL::CNullTraits>
{
	using Base = ATL::CWindowImpl<HiddenWindow<Observer>, ATL::CWindow, ATL::CNullTraits>;

	public:
	explicit HiddenWindow(Observer& observer) :
		m_pObserver(&observer)
	{
	}

	HWND Create()
	{
		return ATL::CWindowImpl<HiddenWindow<Observer>, ATL::CWindow, ATL::CNullTraits>::Create(HWND_MESSAGE);
	}

	BEGIN_MSG_MAP(HiddenWindow)
		MESSAGE_HANDLER(WM_APP, OnMessage)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
	END_MSG_MAP()

	void Notify()
	{
		Base::PostMessage(WM_APP);
	}

	void ClearTimer()
	{
		Base::KillTimer(1);
	}

	void SetTimerMs(unsigned ms)
	{
		Base::SetTimer(1, ms);
	}

	LRESULT OnMessage(UINT, WPARAM, LPARAM, BOOL&)
	{
		m_pObserver->OnMessage();
		return 0;
	}

	LRESULT OnTimer(UINT, WPARAM, LPARAM, BOOL&)
	{
		m_pObserver->OnTimer();
		return 0;
	}

private:
	Observer* m_pObserver;
};

} // namespace detail

class GuiExecutorBase
{
public:
	virtual void OnMessage() = 0;
	virtual void OnTimer() = 0;

protected:
	virtual ~GuiExecutorBase();
};

class GuiExecutor :
	private ExecutorBase,
	private GuiExecutorBase
{
public:
	typedef std::chrono::steady_clock Clock;
	typedef Clock::time_point TimePoint;
	typedef Clock::duration Duration;

	GuiExecutor();
	virtual ~GuiExecutor();

	template <typename Fn>
	auto Call(Fn fn)
	{
		assert(!IsExecutorThread());
		typedef decltype(fn()) R;
		std::packaged_task<R ()> task(fn);
		m_q.Push([&task]() { task(); });
		m_wnd.Notify();
		return task.get_future().get();
	}

	template <typename Fn>
	auto CallAsync(Fn fn)
	{
		typedef decltype(fn()) R;
		auto pTask = std::make_shared<std::packaged_task<R ()>>(fn);
		m_q.Push([pTask]() { (*pTask)(); });
		m_wnd.Notify();
		return pTask->get_future();
	}

	ScheduledCall CallAt(const TimePoint& at, std::function<void ()> fn);
	ScheduledCall CallAfter(const Duration& interval, std::function<void ()> fn);
	ScheduledCall CallEvery(const Duration& interval, std::function<void ()> fn);

    virtual void Cancel(const ScheduledCall& call) override;

	bool IsExecutorThread() const;
	bool IsIdle() const;
	void Synchronize();

private:
	typedef TimedCalls::CallData CallData;

	virtual void OnMessage() override;
	virtual void OnTimer() override;
	void ResetTimer();

	std::thread::id m_guiThreadId;
	detail::HiddenWindow<GuiExecutorBase> m_wnd;
	SynchronizedQueue<std::function<void ()>> m_q;
	TimedCalls m_scheduledCalls;
};

bool GuiWaitFor(std::function<bool ()> pred);

} // namespace fusion
