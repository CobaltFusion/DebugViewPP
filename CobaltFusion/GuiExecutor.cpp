// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

// #include "stdafx.h"
#include <cassert>
#include "Win32/Win32Lib.h"
#include "CobaltFusion/GuiExecutor.h"
#include "CobaltFusion/dbgstream.h"

namespace fusion {

namespace {

void DoCall(std::function<void()>&& fn)
{
    try
    {
        fn();
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

} // namespace

GuiExecutorBase::~GuiExecutorBase() = default;

GuiExecutor::GuiExecutor() :
    m_guiThreadId(std::this_thread::get_id()),
    m_wnd(*this)
{
    assert(::IsGUIThread(FALSE));
    m_wnd.Create();
}

GuiExecutor::~GuiExecutor()
{
    if (IsExecutorThread())
    {
        m_wnd.DestroyWindow();
    }
    else
    {
        Call([this] { m_wnd.DestroyWindow(); });
    }
}

ScheduledCall GuiExecutor::CallAt(const TimePoint& at, std::function<void()> fn)
{
    unsigned id = GetCallId();
    CallAsync([this, id, at, fn]() {
        m_scheduledCalls.Insert(GuiExecutor::CallData(id, at, fn));
        ResetTimer();
    });
    return MakeScheduledCall(id);
}

ScheduledCall GuiExecutor::CallAfter(const Duration& interval, std::function<void()> fn)
{
    return CallAt(std::chrono::steady_clock::now() + interval, fn);
}

ScheduledCall GuiExecutor::CallEvery(const Duration& interval, std::function<void()> fn)
{
    assert(interval > Duration::zero());

    unsigned id = GetCallId();
    auto at = std::chrono::steady_clock::now() + interval;
    CallAsync([this, id, at, interval, fn]() {
        m_scheduledCalls.Insert(GuiExecutor::CallData(id, at, interval, fn));
        ResetTimer();
    });
    return MakeScheduledCall(id);
}

void GuiExecutor::Cancel(const ScheduledCall& call)
{
    unsigned id = GetId(call);
    if (IsExecutorThread())
    {
        m_scheduledCalls.Remove(id);
        ResetTimer();
    }
    else
    {
        Call([this, id]() {
            m_scheduledCalls.Remove(id);
            ResetTimer();
        });
    }
}

bool GuiExecutor::IsExecutorThread() const
{
    return std::this_thread::get_id() == m_guiThreadId;
}

bool GuiExecutor::IsIdle() const
{
    return m_q.Empty();
}

void GuiExecutor::Synchronize()
{
    Call([] {});
}

void GuiExecutor::OnMessage()
{
    assert(IsExecutorThread());

    while (!m_q.Empty())
    {
        DoCall(m_q.Pop());
    }
}

void GuiExecutor::OnTimer()
{
    ResetTimer();
}

void GuiExecutor::ResetTimer()
{
    assert(IsExecutorThread());

    m_wnd.ClearTimer();
    while (!m_scheduledCalls.IsEmpty())
    {
        auto now = std::chrono::steady_clock::now();
        auto at = m_scheduledCalls.NextDeadline();
        if (at <= now)
        {
            DoCall(m_scheduledCalls.Pop().fn);
        }
        else
        {
            m_wnd.SetTimerMs(static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(at - now).count()));
            break;
        }
    }
}

bool GuiWaitFor(std::function<bool()> pred)
{
    while (!pred())
    {
        MSG msg;
        switch (GetMessage(&msg, nullptr, 0, 0))
        {
        case -1: Win32::ThrowLastError("GetMessage");
        case 0: return false;
        default: break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

} // namespace fusion
