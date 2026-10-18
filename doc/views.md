# Views, filters and the log buffer

DebugView++ keeps everything it receives in one **log buffer**. What you see on screen is a **view** on that buffer. This page
explains how the two relate, because it explains why filters, 'Clear View' and 'Reset View' behave the way they do.

## The log buffer and the views

```
 sources                      log buffer                      views (the tabs)
 (OutputDebugString,    -->   every message, once,    -->    View 1: filters A, shows a subset
  kernel, files, pipes,       in the order it                View 2: filters B, shows a subset
  UDP, ...)                   was received                   View 3: no filters, shows everything
```

- The **log buffer** holds every message that was received, exactly once, unfiltered. All views share it.
  Messages are stored compressed, so a buffer can hold a lot of lines, but it still uses memory, see [History size](#history-size).
- A **view** (a tab) does not hold messages. It holds a list of *which lines of the log buffer to show*, plus its own settings.
  Every view has its own:
  - **filters** (include, exclude, highlight, ...), see [Filters](../README.md#filters),
  - **first line**: the first line of the log buffer that the view is allowed to show (see [Reset and clear](#reset-and-clear)),
  - bookmarks, the columns that are shown (and their width and order), clock time on/off and process colors on/off.
- A new view is created with **File, New View** (the filter dialog opens first), or with **Copy View** (same filters, you
  can change them in the dialog). Closing a view removes only that view, the log buffer and the other views are not affected.
  A view that is created later also shows the messages that were received before it existed: it is filled from the log buffer.

If there is just one view, you do not see tabs. The tabs appear when you add a second view.

## How a message reaches a view

1. A source (for example the Win32 OutputDebugString capture, a file that is tailed, or a pipe) receives a message.
2. The message is added to the **log buffer**. This always happens, whatever the filters are.
3. **Each view** looks at the new message and decides, using *its own* filters, if the message is shown in that view.
   If it is, the view remembers the line number of the message in the log buffer. Highlight colors are applied when the
   line is drawn.

The same message can be shown in one view and be hidden in another, because it is the view that decides.

## Filters work on the view, not on the buffer

This is the most important thing to know about filters:

- **A filter never removes a message from the log buffer.** An exclude filter only hides the line in that view. The message is
  still in the buffer, and still uses memory. It is still there in the other views, and 'Save Log' saves it.
- **Changing the filters of a view is applied to the whole buffer again.** When you press OK in the filter dialog the view is
  rebuilt: every line of the log buffer (from the first line of the view) is tested against the new filters. Lines that were
  hidden by an old exclude filter come back when you remove that filter, and lines that you included are hidden again
  when you add an exclude filter. You do not lose messages by experimenting with filters.
- **'once' filters** start again: after the view is rebuilt, or cleared, the first matching line is included again.
- Highlight, token, track, stop, beep and bookmark filters do not hide anything, they only change how lines are shown, or what
  happens when a matching line arrives. See [Filters](../README.md#filters) for what each type does.
- A **clear** filter acts on the view: when a matching message arrives, the view is cleared, like **Clear View** (see below).
  Only the view that has the filter is cleared, the log buffer is not changed.

What a filter *cannot* bring back is a message that is no longer in the log buffer: after **Clear Log**, after **Crop Log to
Selection**, or when it was removed because of the **History Size**.

## Reset and clear

There are three commands that change *which part of the log buffer a view shows* (they are in the **View** menu and in the context
menu of a view), and one that removes data from the buffer (in the **Log** menu).

| Command                    | What it does                                                                                                   | Affects        | Messages are deleted? |
|----------------------------|----------------------------------------------------------------------------------------------------------------|----------------|-----------------------|
| **Clear View**             | The view is emptied, and shows only messages that are received *after* this moment. The first line of the view becomes the next message to arrive. | this view      | no                    |
| **Reset View**             | The first line of the view becomes the first line of the log buffer, so the view shows everything that is in the buffer (that matches the filters). | this view      | no                    |
| **Reset View to this line**| The first line of the view becomes the line that is selected (focused). Everything before it is not shown in this view. | this view      | no                    |
| **Clear Log** (Ctrl+Shift+X) | Removes all messages from the log buffer and empties all views.                                              | all views      | **yes**               |

In other words: *Clear View*, *Reset View* and *Reset View to this line* only move the **first line** of one view. The log
buffer is not changed, and the other views do not notice.

Some consequences, and some examples:

- **'Clear View' is not 'Clear Log'.** After *Clear View* the messages are still in the buffer. *Reset View* shows them again, and
  a second view still shows them. Use *Clear Log* if you really want them gone (for example to free memory).
- **To start a test with an empty screen**: use *Clear View* (or Ctrl+X in the view). It is quick and non-destructive, you can
  still go back with *Reset View*.
- **To look at only the part of the log after an event**: select the line where it starts and choose *Reset View to this line*.
  Lines before it are hidden in this view. Choose *Reset View* to get them back.
- **To see everything again after a Clear View**: *Reset View*.
- **Filters and reset combine**: the view shows the lines from its first line on that match its filters. *Reset View to this
  line* uses the line number in the log buffer, so the line stays the first line when you change the filters.
- Bookmarks that are on lines that stay in the view are kept when a view is rebuilt.
- *Crop Log to Selection* (Log menu) keeps only the selected lines in the log buffer and deletes the rest. This is permanent,
  unlike the commands that change a view.

If a program sends the message `DBGVIEWCLEAR` (the same message Sysinternals DebugView understands), DebugView++ clears the log
as if you used *Clear Log*: that clears the log buffer and all views, not only one view.

## History size

By default the log buffer keeps every message until you clear it, so a long-running session uses more and more memory.
**Log, History Size...** limits the number of messages that are kept: when there are more, the oldest messages are removed from the
log buffer (and from every view). Messages are removed in blocks of 400, so a few hundred more messages than the size you set
may be kept, but never fewer. The setting is remembered between runs. 'Unlimited' (0) keeps everything.

Because the history size works on the log buffer, it removes messages that are hidden by filters too, and 'Reset View' can only bring back
the lines that are still in the buffer.

## Saving

- **Save View** (Ctrl+S) saves the lines that are *shown in the current view*: filters, and the first line of the view,
  decide what is in the file. **Save Selection** (Alt+S) saves only the lines that are selected in the view.
- **Save Log** (Ctrl+Shift+S) saves *the whole log buffer*, whatever the views show.
- **Save Configuration** saves the setup of your session (the views with their filters and columns, and the sources) in a
  `.dbconf` file, not the messages. Use **Load Configuration** (or give the file on the command line) to get the same views back.

## Link views

If you use several views, **Options, Link Views** makes the selected line in one view the selected line in the view you switch
to. This works because all views share the log buffer: the line number in the buffer is the same in every view. If the line is
hidden by the filters of the other view, the nearest line is selected.

A common use: one view with the noisy messages filtered out and auto scroll on, and one view without filters. When something
happens, select a line, switch to the unfiltered view and you see the same line in its full context.
