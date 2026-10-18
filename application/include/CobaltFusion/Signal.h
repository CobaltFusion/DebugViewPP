// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace fusion {

// A small thread-safe signal/slot implementation, with the shape of boost::signals2 that DebugView++ used:
//
//   Signal<void(int)> signal;
//   Connection c = signal.connect([](int i) { ... });
//   signal(42);
//   c.disconnect();
//
// - Slots are called in the order they were connected, on the thread that calls the signal.
// - connect, disconnect and the call itself can be done from any thread, also from inside a slot:
//   the call works on a snapshot of the slots (taken under a lock), the slots are invoked outside the lock.
//   A slot that is disconnected while a call is in progress can still be called by that call, once.
// - The result of a call is the result of the last slot (a default constructed value if there are no slots).
// - A Connection does not disconnect when it is destroyed, and it can outlive the signal.

class Connection
{
public:
    Connection() = default;

    void disconnect()
    {
        if (auto disconnector = m_disconnector.lock())
        {
            (*disconnector)();
        }
        m_disconnector.reset();
    }

    bool connected() const
    {
        return !m_disconnector.expired();
    }

private:
    template <typename Signature>
    friend class Signal;

    explicit Connection(std::weak_ptr<std::function<void()>> disconnector) :
        m_disconnector(std::move(disconnector))
    {
    }

    std::weak_ptr<std::function<void()>> m_disconnector;
};

template <typename Signature>
class Signal;

template <typename R, typename... Args>
class Signal<R(Args...)>
{
public:
    using result_type = R;
    using slot_type = std::function<R(Args...)>;

    Signal() :
        m_state(std::make_shared<State>())
    {
    }

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    Connection connect(slot_type slot)
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        const auto id = m_state->nextId++;
        // the disconnector only refers to the state, so a Connection that outlives the signal is harmless
        auto disconnector = std::make_shared<std::function<void()>>([weakState = std::weak_ptr<State>(m_state), id] {
            if (auto state = weakState.lock())
            {
                state->Remove(id);
            }
        });
        m_state->slots.push_back(Entry{id, std::make_shared<slot_type>(std::move(slot)), disconnector});
        return Connection(disconnector);
    }

    void disconnect_all_slots()
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        m_state->slots.clear();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        return m_state->slots.empty();
    }

    R operator()(Args... args) const
    {
        const auto slots = Snapshot();
        if constexpr (std::is_void_v<R>)
        {
            for (const auto& slot : slots)
            {
                (*slot)(args...);
            }
        }
        else
        {
            R result{};
            for (const auto& slot : slots)
            {
                result = (*slot)(args...);
            }
            return result;
        }
    }

private:
    struct Entry
    {
        size_t id;
        std::shared_ptr<slot_type> slot;
        std::shared_ptr<std::function<void()>> disconnector; // keeps the Connection 'connected' while the slot is in the list
    };

    struct State
    {
        std::mutex mutex;
        std::vector<Entry> slots;
        size_t nextId = 0;

        void Remove(size_t id)
        {
            std::lock_guard<std::mutex> lock(mutex);
            slots.erase(std::remove_if(slots.begin(), slots.end(), [id](const Entry& e) { return e.id == id; }), slots.end());
        }
    };

    std::vector<std::shared_ptr<slot_type>> Snapshot() const
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        std::vector<std::shared_ptr<slot_type>> result;
        result.reserve(m_state->slots.size());
        for (const auto& entry : m_state->slots)
        {
            result.push_back(entry.slot);
        }
        return result;
    }

    std::shared_ptr<State> m_state;
};

} // namespace fusion
