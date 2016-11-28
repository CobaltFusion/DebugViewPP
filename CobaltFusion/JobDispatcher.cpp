// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include <future>
#include "CobaltFusion/JobDispatcher.h"

namespace fusion {

JobDispatcher::JobDispatcher() :
	m_stop(false)
{
}

JobDispatcher::~JobDispatcher() = default;

void JobDispatcher::Flush()
{
	std::lock_guard<std::mutex> lock(m_stopMutex);
	if (m_stop == false)
	{
		std::promise<bool> flush;
		auto flushed = flush.get_future();
		Queue([&flush] ()
		{ 
			flush.set_value(true);
		});
		flushed.wait();
	}
}

void JobDispatcher::Queue(Job job)
{       
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_jobQueue.push_back(job);
	}
		
	std::lock_guard<std::mutex> lock(m_executionMutex);
	m_condition.notify_one();
}

boost::signals2::connection JobDispatcher::SubscribeToExceptionEvent(std::function<void(std::exception_ptr)> function)
{
	return m_exceptionOccurredSignal.connect(function);
}

bool JobDispatcher::HasJobs()
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	return !m_jobQueue.empty();
}

void JobDispatcher::ClearJobs()
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	m_jobQueue.clear();
}

Job JobDispatcher::NextJob()
{
	Job job;

	std::lock_guard<std::mutex> lock(m_queueMutex);
	if (!m_jobQueue.empty())
	{
		job = m_jobQueue.front();
		m_jobQueue.pop_front();
	}
	return job;
}

BackgroundDispatcher::BackgroundDispatcher()
{
	m_thread.reset(new std::thread(&BackgroundDispatcher::Run, this));
}

BackgroundDispatcher::~BackgroundDispatcher()
{
	Stop();
}

void BackgroundDispatcher::Run()
{
	for (;;)
	{
		std::unique_lock<std::mutex> lock(m_executionMutex);
		if (!HasJobs() || !m_stop)
		{
			// wait for a job to be queued
			m_condition.wait(lock, [this] () { return HasJobs() || m_stop; });
		}

		if (m_stop)
		{
			break;
		}

		lock.unlock();

		auto job = NextJob();
		while (job && !m_stop)
		{
			try
			{
				job();
			}
			catch (...)
			{
				m_exceptionOccurredSignal(std::current_exception());
			}
			job = NextJob();
		}
	}
}

void BackgroundDispatcher::Stop()
{
	if (m_stop)
	{
		return;
	}

	{
		std::lock_guard<std::mutex> stopLock(m_stopMutex);
		std::lock_guard<std::mutex> executeLock(m_executionMutex);
		m_stop = true;
		m_condition.notify_one();
	}

	if (m_thread->joinable())
	{
		m_thread->join();
	}

	m_thread.reset();
}

OnDemandDispatcher::OnDemandDispatcher()
{
}

OnDemandDispatcher::~OnDemandDispatcher()
{
}

void OnDemandDispatcher::Queue(Job job)
{
	JobDispatcher::Queue(job);
	m_jobQueuedSignal();
}

void OnDemandDispatcher::ExecuteQueuedJobs()
{
	// we only want to do the jobs currently in the queue, so queue a 'stop job'
	bool done = false;
	JobDispatcher::Queue([&done] () {
		done = true;
	});
	
	// we must catch indivial job exceptions to make sure we
	// execute all subsequent jobs and consume the 'stop job' 
	auto job = NextJob();
	while (job && !done)
	{
		try
		{
			job();
		}
		catch (...)
		{
			m_exceptionOccurredSignal(std::current_exception());
		}
		job = NextJob();
	}
}

boost::signals2::connection OnDemandDispatcher::SubscribeToJobQueuedEvent(std::function<void()> function)
{
	return m_jobQueuedSignal.connect(function);
}

} // namespace fusion
