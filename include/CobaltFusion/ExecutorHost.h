// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <thread>
#include <chrono>

namespace fusion {

class IExecutor
{
public:
	typedef std::chrono::steady_clock Clock;
	virtual void Call(std::function<void()> fn) = 0;
	virtual void CallAsync(std::function<void()> fn) = 0;
	virtual void CallAfter(const Clock::duration& interval, std::function<void()> fn) = 0;
	virtual bool IsExecutorThread() const = 0;
};

class GuiExecutor;

class GuiExecutorHost : public IExecutor
{
public:
	GuiExecutorHost();
	virtual void Call(std::function<void()> fn) override;
	virtual void CallAsync(std::function<void()> fn) override;
	virtual void CallAfter(const Clock::duration& interval, std::function<void()> fn) override;
	virtual bool IsExecutorThread() const override;
private:
	std::unique_ptr<GuiExecutor> m_executor;
};

class ActiveExecutor;

class ActiveExecutorHost : public IExecutor
{
public:
	ActiveExecutorHost();
	virtual void Call(std::function<void()> fn) override;
	virtual void CallAsync(std::function<void()> fn) override;
	virtual void CallAfter(const Clock::duration& interval, std::function<void()> fn) override;
	virtual bool IsExecutorThread() const override;
private:
	std::unique_ptr<ActiveExecutor> m_executor;
};

} // namespace fusion
