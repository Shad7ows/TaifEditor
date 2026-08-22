# Taif Native Terminal Module

**Status:** Implemented and validated in Phase 8
**Scope:** Windows ConPTY and POSIX PTY transports, portable session contract, VT stream handling, direct-paint terminal viewport, and dock integration

## Design intent

The System Terminal is a terminal emulator, not a process-output text editor. It must accept direct keyboard input, maintain cursor-addressable cell state, honor the terminal controls commonly emitted by interactive shells, and retain one terminal session when its dock is hidden or shown. The module therefore separates platform process I/O from terminal semantics and widget painting.

| Requirement | Design response |
|---|---|
| Direct interactive input | `TerminalView` owns focus and encodes keyboard events as terminal input bytes. |
| Shell-compatible rendering | `TerminalScreenModel` owns a cursor-addressable cell grid, scrollback, and attributes. |
| Incremental terminal output | `VtStreamParser` retains decoder and escape-sequence state across arbitrary byte boundaries. |
| Platform isolation | `ITerminalBackend` defines process transport; Windows uses ConPTY and Linux/macOS use a native POSIX PTY. |
| Qt lifecycle safety | `TerminalSessionController` owns state transitions and signal delivery on the GUI thread. |
| Existing UI preservation | `TConsole` embeds the viewport in its existing dock-facing `QPlainTextEdit` base without changing `Taif::setupUI()`. |

## Component contracts

### `TerminalScreenModel`

`TerminalScreenModel` is a QWidget-free, process-free terminal state machine. It owns the visible grid, primary and alternate buffers, scrollback, cursor and saved cursor, attributes, title, and scroll region. It is the only module permitted to mutate terminal cell state.

| Contract | Invariant |
|---|---|
| Grid dimensions | Columns are at least 2 and rows are at least 1 after construction or resize. |
| Cell content | Each grid cell always has printable text, defaulting to a single space. |
| Cursor | Cursor row and column are clamped to the active grid. |
| Scrollback | Only primary-screen scrolling records scrollback; alternate-screen scrolling does not. The first hidden-to-visible layout resize never creates artificial blank history before the shell has emitted content. |
| Alternate screen | Entering preserves primary grid/cursor and presents a blank alternate grid; leaving restores primary state. |
| Title and attributes | These are model state, independent of transport and painting. |

### `VtStreamParser`

`VtStreamParser` accepts arbitrary `QByteArray` chunks, decodes UTF-8 with `QStringDecoder`, and feeds characters into an explicit finite state machine. It recognizes printable text, C0 controls, Escape sequences, CSI, OSC, and OSC escape termination. Parser state cannot be reset merely because a transport read ends.

| Sequence category | Current support |
|---|---|
| C0 controls | Backspace, tab, carriage return, and line feed. |
| Cursor CSI | `A`, `B`, `C`, `D`, `E`, `F`, `G`, `H`, `f`, `s`, and `u`. |
| Editing CSI | `J`, `K`, `L`, `M`, `P`, `S`, `T`, and `X`. |
| Style CSI | `m` with reset, bold, underline, inverse, normal/bright ANSI colors, indexed colors, and RGB colors. |
| Regions and modes | `r`; private `?25`, `?47`, `?1047`, and `?1049`. |
| OSC | `0` and `2` title sequences, BEL- or `ESC \\`-terminated. |
| Unsupported sequences | Counted by `ignoredSequenceCount()` and otherwise safely ignored. |

### `TerminalView`

`TerminalView` is an LTR direct-paint viewport. It converts the model’s cells and attributes into a fixed-grid display, maps selection into scrollback/grid text for copy, and emits keyboard byte sequences through `terminalInput`. A debounced geometry calculation emits `gridSizeChanged` only when a meaningful cell-grid dimension changes.

> **Visual-row invariant:** The view treats the terminal as one contiguous sequence: `scrollback` rows followed by active-grid rows. The vertical scrollbar addresses a viewport over that sequence, with a maximum of `max(0, totalVisualRows - visibleRows)`. Tail-following therefore shows the complete active screen, while scrolling upward renders and copies older rows instead of clamping offsets into the active grid. The cursor is painted only while that tail viewport is current.

Mouse events arrive at `QAbstractScrollArea`’s viewport child. `TerminalView::viewportEvent()` forwards press, move, and release events into the terminal cell-selection path, keeping selection and copy coordinates on the same visual-row mapping as painting.

> **Directionality invariant:** The terminal grid is always LTR. Arabic/RTL application layout must not reverse cursor direction, cell order, or selection behavior inside the terminal viewport.

### `ITerminalBackend`, `WindowsConPtyBackend`, and `PosixPtyBackend`

`ITerminalBackend` is the transport seam. It receives a program, arguments, working directory, and initial grid; it provides input writes, grid resizing, cancellation, bounded shutdown, output/error reporting, and final completion notification.

`WindowsConPtyBackend` creates a pseudoconsole from two inherited pipe ends, supplies its `HPCON` through `PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE`, and starts the requested program with `CreateProcessW`. It owns the process, primary thread, pipe ends, pseudoconsole, and reader thread. Output is emitted through queued Qt delivery so UI code never paints from the reader thread.

`PosixPtyBackend` is compiled on Unix platforms and is selected by `TerminalSessionController` outside Windows. It creates the terminal with `forkpty`, changes to the requested working directory in the child, executes the requested shell through `execvp`, and reads the PTY master from a dedicated worker thread. Input writes target that master directly; grid resize uses `TIOCSWINSZ`; cancellation signals the child process group before bounded shutdown escalates to `SIGKILL`. Reader output and completion are delivered to Qt through queued GUI-thread notifications. Linux links `libutil` for `forkpty`; macOS obtains that API from its system libraries.

| Lifecycle operation | Required behavior |
|---|---|
| `start()` | Rejects an active backend or an unavailable executable; creates handles and starts a reader only after process creation succeeds. |
| `writeInput()` | Writes only while active and while the input pipe is valid. |
| `resize()` | Uses `ResizePseudoConsole` only for a valid `HPCON` and normalized positive grid. |
| `requestStop()` | Sends Ctrl+C once while the session remains marked active. |
| `shutdown()` | Attempts graceful termination, escalates after the supplied bound, stops reader work, closes handles, joins the reader, then closes the pseudoconsole. |
| natural child exit | Reader drains available output, detects child completion, and emits terminal completion exactly once. |
| Unix resize and cancellation | `PosixPtyBackend` applies `TIOCSWINSZ` to the PTY master and signals the child process group, retaining the same controller-level state contract as ConPTY. |

The backend’s reader deliberately polls available pipe bytes and checks the child process when no bytes remain. After child completion, it keeps a short bounded idle grace interval to drain ConPTY’s late final frames before reporting completion. This prevents one-shot sessions from losing their final output while also preventing a pseudoconsole pipe endpoint from keeping the session artificially alive.

The pseudoconsole-side pipe ends are released immediately after `CreatePseudoConsole` succeeds, before the hosted process is created. The hosted process is created from a mutable command line with `EXTENDED_STARTUPINFO_PRESENT` and the pseudoconsole attribute list. This preserves the transport’s sole ownership of the host-side input-write and output-read endpoints.

`STARTF_USESTDHANDLES` is set with the zero-initialized standard-handle fields before `CreateProcessW`. This is mandatory for this host because otherwise Windows can duplicate the parent process standard streams into a console child even when `bInheritHandles` is `FALSE`. That failure mode produces an empty terminal grid while `cmd.exe` writes its banner, prompt, and command output to the parent console instead of the ConPTY output pipe. [1]

### `TerminalSessionController`

`TerminalSessionController` is the GUI-thread lifecycle owner. It constructs `WindowsConPtyBackend` under `Q_OS_WIN` and `PosixPtyBackend` under `Q_OS_UNIX`, then translates either backend into the state sequence below and prevents repeated terminal completion delivery.

| State | Meaning | Allowed principal transition |
|---|---|---|
| `Idle` | No started terminal session. | `start()` to `Starting`. |
| `Starting` | A validated launch request is entering the backend. | Successful start to `Running`; error to `Failed`. |
| `Running` | Input and resize requests are forwarded. | `cancel()` to `Stopping`; backend finish to `Finished` or `Failed`. |
| `Stopping` | Ctrl+C was requested and escalation timer is armed. | Backend finish to terminal result; escalation performs bounded shutdown. |
| `Finished` | Session ended normally. | A later `start()` may begin a new session. |
| `Failed` | Launch or execution ended abnormally. | A later `start()` may begin a new session. |

## TConsole dock integration

`TConsole` is the stable dock-facing base class. Calling `enableNativeTerminal()` hides the inherited document scrollbars, creates a `TerminalView` as the `QPlainTextEdit` viewport child, creates the controller, wires input/output/resize signals, and forces LTR layout. `resizeEvent()` resizes the embedded viewport to `viewport()->rect()`; this is the only geometry authority for the view.

`focusNativeTerminal()` is the explicit focus boundary for the embedded interactive surface. `DockableConsoleToolFactory::showAndActivate()` calls it only when a dock widget is a native-enabled `TConsole`; all non-native consoles retain the established generic focus path. `TerminalView` also claims `Qt::MouseFocusReason` before starting a left-button selection, so either dock activation or a grid click directs typing to the terminal input encoder.

Native-mode `startCmd()` selects the system default shell and forwards the current terminal grid size. Before the first shell launch, `Taif::showAndRaiseDock()` supplies the opened project root when available, otherwise the active editor file directory, and finally the process working directory as a fallback. The factory intentionally defers the first native shell launch through the post-show event turn, after the dock has a real multi-row viewport. Starting ConPTY while the initially hidden dock is `2x1` creates a tiny screen buffer whose later padding is perceived as mostly blank terminal rows.

Native-mode `clear()` clears the grid and `appendPlainTextThreadSafe()` injects bytes into the terminal parser. Non-native methods remain virtual to preserve the `InlinePromptConsole` contract. The public output-limit constants remain on `TConsole` for compatibility with established console regressions; transcript enforcement is supplied by `InlinePromptConsole`.

## Test obligations

A behavior change must include focused regression coverage appropriate to its layer before it may be considered complete.

| Change area | Minimum regression coverage |
|---|---|
| Model mutation | Cursor, grid, scrollback, erase, scroll region, or alternate-screen assertions against `TerminalScreenModel`. |
| Parser support | Split byte/escape input plus observable cell, title, attribute, or cursor state. |
| Transport/lifecycle | Interactive `cmd.exe` input/output round trip whose expected directory output cannot be satisfied by input echo; invalid-start error, cancellation, and bounded shutdown as applicable. |
| View behavior | Keyboard input encoding, selection/copy, resize emission, LTR rendering, and scrollback top/tail presentation behavior. |
| Dock integration | Native mode is enabled for the System Terminal, project-directory configuration is accepted before first start, first activation yields a multi-row grid, dock visibility does not recreate the session, and the terminal remains a tab peer with Problems and Alif Output. |
| Platform transport | Windows runs the ConPTY `cmd.exe` round trip. Linux/macOS run an interactive `/bin/sh -i` PTY round trip that sends input and observes its output. |

The current focused target is `tests/ui/ui_tests.pro`. Its Phase 8 execution includes parser/model coverage, a scrollback regression that selects historical rows at the top and live output at the tail, a Windows interactive ConPTY `cmd.exe` input/output round trip, a Unix interactive `/bin/sh -i` PTY round trip, native dock-to-viewport focus and grid-rendering coverage, inline-prompt boundary/history coverage, existing dock persistence coverage, and the broader editor/UI suite. The repository-wide Windows completion gate remains `scripts\\validate_windows.cmd`; Unix and macOS runners must also execute the focused UI target to exercise their native backend.

## References

[1] [Windows Terminal discussion: standard-handle routing with ConPTY](https://github.com/microsoft/terminal/discussions/15814)

## Maintenance rules

Future maintainers must not substitute `QProcess` text output for the System Terminal path, route System Terminal input through a separate line editor, make the terminal viewport RTL, or block the GUI thread while waiting for a child process. New platform transports must implement `ITerminalBackend` and preserve the controller state contract. New terminal control support must update the compatibility table and add split-stream coverage.
