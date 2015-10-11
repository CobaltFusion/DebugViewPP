// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include <algorithm>
#include <boost/atomic.hpp>
#include "CobaltFusion/Executor.h"
#include "CobaltFusion/dbgstream.h"

namespace fusion {

ExecutorBase::~ExecutorBase()
{
}

unsigned ExecutorBase::GetId(const ScheduledCall& call) const
{
	return call.GetId();
}

ScheduledCall ExecutorBase::MakeScheduledCall(unsigned id)
{
	return ScheduledCall(*this, id);
}

unsigned ExecutorBase::GetCallId()
{
	static boost::atomic<unsigned> id(0);
	return ++id;
}

bool TimedCalls::IsEmpty() const
{
	return m_scheduledCalls.empty();
}

void TimedCalls::Insert(CallData&& call)
{
	auto it = std::lower_bound(m_scheduledCalls.begin(), m_scheduledCalls. end(), call, [](const CallData& a, const CallData& b) { return a.at > b.at; });
	m_scheduledCalls.insert(it, call);
}

void TimedCalls::Remove(unsigned id)
{
	auto it = std::find_if(m_scheduledCalls.begin(), m_scheduledCalls.end(), [id](const CallData& cd) { return cd.id == id; });
	if (it != m_scheduledCalls.end())
		m_scheduledCalls.erase(it);
}

TimedCalls::TimePoint TimedCalls::NextDeadline() const
{
	assert(!m_scheduledCalls.empty());
	return m_scheduledCalls.back().at;
}

TimedCalls::CallData TimedCalls::Pop()
{
	TimedCalls::CallData call(std::move(m_scheduledCalls.back()));
	m_scheduledCalls.pop_back();
	if (call.interval != Duration::zero())
		Insert(CallData(call.id, call.at + call.interval, call.interval, call.fn));
	return call;
}

TimedCalls::CallData::CallData(unsigned id, TimePoint at, std::function<void ()> fn) :
	id(id),
	at(at),
	interval(Duration::zero()),
	fn(fn)
{
}

TimedCalls::CallData::CallData(unsigned id, TimePoint at, Duration interval, std::function<void ()> fn) :
	id(id),
	at(at),
	interval(interval),
	fn(fn)
{
}

ScheduledCall::ScheduledCall() :
	pExec(nullptr)
{
}

ScheduledCall::ScheduledCall(ExecutorBase& exec, unsigned id) :
	pExec(&exec), id(id)
{
}

void ScheduledCall::Cancel()
{
	if (!pExec)
		return;

	pExec->Cancel(*this);
	pExec = nullptr;
}

unsigned ScheduledCall::GetId() const
{
	return id;
}

ScopedScheduledCall::ScopedScheduledCall()
{
}

ScopedScheduledCall::ScopedScheduledCall(const ScheduledCall& call) :
	m_call(call)
{
}

ScopedScheduledCall::ScopedScheduledCall(ScopedScheduledCall&& call) :
	m_call(call.m_call)
{
	call.m_call = ScheduledCall();
}

ScopedScheduledCall::~ScopedScheduledCall()
{
	m_call.Cancel();
}

ScopedScheduledCall& ScopedScheduledCall::operator=(const ScheduledCall& call)
{
	m_call.Cancel();
	m_call = call;
	return *this;
}

ScopedScheduledCall& ScopedScheduledCall::operator=(ScopedScheduledCall&& call)
{
	m_call.Cancel();
	m_call = call.m_call;
	call.m_call = ScheduledCall();
	return *this;
}

void ScopedScheduledCall::Cancel()
{
	m_call.Cancel();
}

Executor::Executor()
{
	SetExecutorThread();
}

bool Executor::IsExecutorThread() const
{
	return boost::this_thread::get_id() == m_threadId;
}

void Executor::RunOne()
{
	SetExecutorThread();
	m_q.Pop()();
}

void Executor::SetExecutorThread()
{
	m_threadId = boost::this_thread::get_id();
}

void Executor::Add(std::function<void ()> fn)
{
	m_q.Push(fn);
}

ScheduledCall TimedExecutor::CallAt(const TimePoint& at, std::function<void ()> fn)
{
	unsigned id = GetCallId();
	Add([this, id, at, fn]()
	{
		m_scheduledCalls.Insert(TimedExecutor::CallData(id, at, fn));
	});
	return MakeScheduledCall(id);
}

ScheduledCall TimedExecutor::CallAfter(const Duration& interval, std::function<void ()> fn)
{
	return CallAt(boost::chrono::steady_clock::now() + interval, fn);
}

ScheduledCall TimedExecutor::CallEvery(const Duration& interval, std::function<void ()> fn)
{
	assert(interval > Duration::zero());

	unsigned id = GetCallId();
	Add([this, id, interval, fn]()
	{
		m_scheduledCalls.Insert(TimedExecutor::CallData(id, boost::chrono::steady_clock::now() + interval, interval, fn));
	});
	return MakeScheduledCall(id);
}

void TimedExecutor::Cancel(const ScheduledCall& call)
{
	unsigned id = GetId(call);
	if (IsExecutorThread())
		m_scheduledCalls.Remove(id);
	else
		Call([this, id]() { m_scheduledCalls.Remove(id); });
}

void TimedExecutor::RunOne()
{
	SetExecutorThread();
	if (!m_scheduledCalls.IsEmpty() && !WaitForNotEmpty(m_scheduledCalls.NextDeadline()))
		m_scheduledCalls.Pop().fn();
	else
		return Executor::RunOne();
}

ActiveExecutor::ActiveExecutor() :
	m_end(false),
	m_thread([this] { Run(); })
{
}

ActiveExecutor::~ActiveExecutor()
{
	Add([this] { m_end = true; });
	m_thread.join();
}

void ActiveExecutor::Run()
{
	while (!m_end)
	{
		try
		{
			RunOne();
		}
		catch (std::exception& e)
		{
			cdbg << "Executor: exception ignored: " << e.what() << "\n";
		}
		catch (...)
		{
			cdbg << "Executor: exception ignored\n";
		}
	}
}

} // namespace fusion
