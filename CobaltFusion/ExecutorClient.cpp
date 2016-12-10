// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include "CobaltFusion/ExecutorClient.h"
#include "CobaltFusion/Executor.h"
#include "CobaltFusion/GuiExecutor.h"

namespace fusion {

GuiExecutorClient::GuiExecutorClient()
	: m_executor(std::make_unique<GuiExecutor>())
{
}

void GuiExecutorClient::Call(std::function<void()> fn)
{
	m_executor->Call(fn);
}

void GuiExecutorClient::CallAsync(std::function<void()> fn)
{
	m_executor->CallAsync(fn);
}

ScheduledCall GuiExecutorClient::CallAfter(const Clock::duration& interval, std::function<void()> fn)
{
	return m_executor->CallAfter(interval, fn);
}

ScheduledCall GuiExecutorClient::CallAt(const Clock::time_point& timepoint, std::function<void()> fn)
{
	return m_executor->CallAt(timepoint, fn);
}

bool GuiExecutorClient::IsExecutorThread() const
{
	return m_executor->IsExecutorThread();
}

void GuiExecutorClient::Synchronize()
{
	m_executor->Synchronize();
}

void GuiExecutorClient::Cancel()
{
	// there is currently no way to know what Async, After or At-call belong to this client
	// and we cannot just store ScheduledCall objects, because the list would only grow 
	throw std::exception("not implemented");
}

ActiveExecutorClient::ActiveExecutorClient()
	: m_executor(std::make_unique<ActiveExecutor>())
{
}

void ActiveExecutorClient::Call(std::function<void()> fn)
{
	m_executor->Call(fn);
}

void ActiveExecutorClient::CallAsync(std::function<void()> fn)
{
	m_executor->CallAsync(fn);
}

ScheduledCall ActiveExecutorClient::CallAfter(const Clock::duration& interval, std::function<void()> fn)
{
	return m_executor->CallAfter(interval, fn);
}

ScheduledCall ActiveExecutorClient::CallAt(const Clock::time_point& timepoint, std::function<void()> fn)
{
	return m_executor->CallAt(timepoint, fn);
}

bool ActiveExecutorClient::IsExecutorThread() const
{
	return m_executor->IsExecutorThread();
}

void ActiveExecutorClient::Synchronize()
{
	m_executor->Synchronize();
}

void ActiveExecutorClient::Cancel()
{

}

} // namespace fusion
