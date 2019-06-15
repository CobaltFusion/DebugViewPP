// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <thread>
#include <chrono>
#include "Executor.h"

namespace fusion {

class IExecutor
{
public:
    virtual ~IExecutor() = default;
    typedef std::chrono::steady_clock Clock;
	virtual void Call(std::function<void()> fn) = 0;
	virtual void CallAsync(std::function<void()> fn) = 0;
	virtual ScheduledCall CallAfter(const Clock::duration& interval, std::function<void()> fn) = 0;
	virtual ScheduledCall CallAt(const Clock::time_point& timepoint, std::function<void()> fn) = 0;
	virtual bool IsExecutorThread() const = 0;
	virtual void Synchronize() = 0;

	// shall cancel all scheduled calls that are not in progress 
	virtual void Cancel() = 0;
};

class GuiExecutor;

class GuiExecutorClient : public IExecutor
{
public:
	GuiExecutorClient();
    virtual ~GuiExecutorClient() = default;
	void Call(std::function<void()> fn) override;
	void CallAsync(std::function<void()> fn) override;
	ScheduledCall CallAfter(const Clock::duration& interval, std::function<void()> fn) override;
	ScheduledCall CallAt(const Clock::time_point& timepoint, std::function<void()> fn) override;
	bool IsExecutorThread() const override;
	void Synchronize() override;
	void Cancel() override;

private:
	std::unique_ptr<GuiExecutor> m_executor;
};

class ActiveExecutor;

class ActiveExecutorClient : public IExecutor
{
public:
	ActiveExecutorClient();
    virtual ~ActiveExecutorClient() = default;
	void Call(std::function<void()> fn) override;
	void CallAsync(std::function<void()> fn) override;
	ScheduledCall CallAfter(const Clock::duration& interval, std::function<void()> fn) override;
	ScheduledCall CallAt(const Clock::time_point& timepoint, std::function<void()> fn) override;
	bool IsExecutorThread() const override;
	void Synchronize() override;
	void Cancel() override;
private:
	std::unique_ptr<ActiveExecutor> m_executor;
};

} // namespace fusion
