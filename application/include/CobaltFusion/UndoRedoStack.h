// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <deque>
#include <utility>

namespace fusion {

// The history of a state, to undo and redo changes of it.
//
// Call Push() with the state *before* it is changed. Undo() replaces the current state by the state before the last change
// (and remembers the current state, to redo), Redo() does the opposite. Making a new change after an undo removes
// the states that could be redone, like in any editor.
template <typename T>
class UndoRedoStack
{
public:
    explicit UndoRedoStack(size_t maxSize = 100) :
        m_maxSize(maxSize)
    {
    }

    void Push(const T& stateBeforeChange)
    {
        m_undo.push_back(stateBeforeChange);
        while (m_undo.size() > m_maxSize)
        {
            m_undo.pop_front(); // forget the oldest change
        }
        m_redo.clear();
    }

    [[nodiscard]] bool CanUndo() const
    {
        return !m_undo.empty();
    }

    [[nodiscard]] bool CanRedo() const
    {
        return !m_redo.empty();
    }

    // returns false (and leaves 'current' alone) if there is nothing to undo
    bool Undo(T& current)
    {
        if (m_undo.empty())
        {
            return false;
        }
        m_redo.push_back(std::move(current));
        current = std::move(m_undo.back());
        m_undo.pop_back();
        return true;
    }

    bool Redo(T& current)
    {
        if (m_redo.empty())
        {
            return false;
        }
        m_undo.push_back(std::move(current));
        current = std::move(m_redo.back());
        m_redo.pop_back();
        return true;
    }

    void Clear()
    {
        m_undo.clear();
        m_redo.clear();
    }

private:
    size_t m_maxSize;
    std::deque<T> m_undo;
    std::deque<T> m_redo;
};

} // namespace fusion
