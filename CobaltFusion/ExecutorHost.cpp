// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include "CobaltFusion/ExecutorHost.h"
#include "CobaltFusion/Executor.h"
#include "CobaltFusion/GuiExecutor.h"

namespace fusion {

GuiExecutorHost::GuiExecutorHost()
	: m_executor(std::make_unique<GuiExecutor>())
{
}

void GuiExecutorHost::Call(std::function<void()> fn)
{
	m_executor->Call(fn);
}

void GuiExecutorHost::CallAsync(std::function<void()> fn)
{
	m_executor->CallAsync(fn);
}

void GuiExecutorHost::CallAfter(const Clock::duration& interval, std::function<void()> fn)
{
	m_executor->CallAfter(interval, fn);
}

bool GuiExecutorHost::IsExecutorThread() const
{
	return m_executor->IsExecutorThread();
}

ActiveExecutorHost::ActiveExecutorHost()
	: m_executor(std::make_unique<ActiveExecutor>())
{
}

void ActiveExecutorHost::Call(std::function<void()> fn)
{
	m_executor->Call(fn);
}

void ActiveExecutorHost::CallAsync(std::function<void()> fn)
{
	m_executor->CallAsync(fn);
}

bool ActiveExecutorHost::IsExecutorThread() const
{
	return m_executor->IsExecutorThread();
}

void ActiveExecutorHost::CallAfter(const Clock::duration& interval, std::function<void()> fn)
{
	m_executor->CallAfter(interval, fn);
}


} // namespace fusion
