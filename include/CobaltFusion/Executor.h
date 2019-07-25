// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <memory>
#include <functional>
#include <thread>
#include <future>
#include <chrono>
#include "CobaltFusion/SynchronizedQueue.h"

namespace fusion {

class ScheduledCall;

// Sean Parent: Concurrency
// https://www.youtube.com/watch?v=zULU6Hhp42w


class ExecutorBase
{
public:
	virtual void Cancel(const ScheduledCall& call) = 0;

protected:
	virtual ~ExecutorBase();
	static unsigned GetId(const ScheduledCall& call);
	ScheduledCall MakeScheduledCall(unsigned id);

	static unsigned GetCallId();
};

class ScheduledCall
{
public:
	ScheduledCall();
	void Cancel();

private:
	friend class ExecutorBase;
	ScheduledCall(ExecutorBase& exec, unsigned id);
	unsigned GetId() const;

	ExecutorBase* pExec;
	unsigned id;
};

class ScopedScheduledCall
{
public:
	ScopedScheduledCall();
	explicit ScopedScheduledCall(const ScheduledCall& call);
	ScopedScheduledCall(ScopedScheduledCall&& call);
	~ScopedScheduledCall();

	ScopedScheduledCall& operator=(const ScheduledCall& call);
	ScopedScheduledCall& operator=(ScopedScheduledCall&& call);

	void Cancel();

private:
	ScheduledCall m_call;
};

class TimedCalls
{
public:
	typedef std::chrono::steady_clock Clock;
	typedef Clock::time_point TimePoint;
	typedef Clock::duration Duration;

	struct CallData
	{
		CallData(unsigned id, TimePoint at, std::function<void()> fn);
		CallData(unsigned id, TimePoint at, Duration interval, std::function<void()> fn);

		unsigned id;
		TimePoint at;
		Duration interval;
		std::function<void()> fn;
	};

	bool IsEmpty() const;
	void Insert(CallData&& call);
	void Remove(unsigned id);
	TimePoint NextDeadline() const;
	CallData Pop();

private:
	std::vector<CallData> m_scheduledCalls;
};

class Executor
{
public:
	Executor();
	virtual ~Executor() {}

	template <typename Fn>
	auto Call(Fn fn)
	{
		assert(!IsExecutorThread());
		std::packaged_task<decltype(fn())()> task(fn);
		Add([&task]() { task(); });
		return task.get_future().get();
	}

	template <typename Fn>
	auto CallAsync(Fn fn)
	{
		auto pTask = std::make_shared<std::packaged_task<decltype(fn())()>>(fn);
		auto f = pTask->get_future();
		Add([pTask]() { (*pTask)(); });
		return f;
	}

	bool IsExecutorThread() const;
	bool IsIdle() const;

	virtual void RunOne();
	void Synchronize();

protected:
	void SetExecutorThread();
	void SetExecutorThread(std::thread::id id);
	void Add(std::function<void()> fn);

	template <typename Clock, typename Duration>
	bool WaitForNotEmpty(const std::chrono::time_point<Clock, Duration>& time) const
	{
		return m_q.WaitForNotEmpty(time);
	}

private:
	SynchronizedQueue<std::function<void()>> m_q;
	std::thread::id m_threadId;
};

class TimedExecutor : private ExecutorBase,
					  public Executor
{
public:
	typedef TimedCalls::Clock Clock;
	typedef TimedCalls::TimePoint TimePoint;
	typedef TimedCalls::Duration Duration;

	ScheduledCall CallAt(const TimePoint& at, std::function<void()> fn);
	ScheduledCall CallAfter(const Duration& interval, std::function<void()> fn);
	ScheduledCall CallEvery(const Duration& interval, std::function<void()> fn);

	void Cancel(const ScheduledCall& call) override;

	void RunOne() override;

private:
	typedef TimedCalls::CallData CallData;

	TimedCalls m_scheduledCalls;
};

class ActiveExecutor : public TimedExecutor
{
public:
	ActiveExecutor();
	~ActiveExecutor() override;

private:
	void Run();

	bool m_end;
	std::thread m_thread;
};

} // namespace fusion
