# Dockable Alif Output and System Terminal

**Status:** Implemented and validated in Phase 8
**Applies to:** Qt 6 / C++17 TaifEditor bottom-tool docks, native System Terminal, and managed Alif execution input/output

## Purpose

TaifEditor exposes three persistent bottom-area tools. The two console roles deliberately use different interaction models: the System Terminal is a full native terminal grid, while Alif Output is a right-to-left transcript with one protected, inline input range. Neither role uses a separate `QLineEdit`.

| Dock title | Dock object name | Widget type | Interaction model |
|---|---|---|---|
| `المشكلات` | `DiagnosticsDock` | `DiagnosticsPanel` | Read-only diagnostics browser. |
| `مخرجات ألف` | `AlifOutputDock` | `InlinePromptConsole` as `TConsole*` | RTL transcript with a protected inline `ألف › ` prompt. |
| `طرفية النظام (CMD/Bash/Zsh)` | `TerminalDock` | native-enabled `TConsole` | LTR VT cell grid backed by a platform terminal transport. |

All tools are `QDockWidget` instances in the bottom docking area. They are movable, floatable, closable, and initially tabified. Hiding a dock only changes visibility; it does not reconstruct the console, discard the transcript, or terminate an active shell.

## Construction and ownership

`DockableConsoleToolFactory` is the sole construction boundary for console docks. It produces a `TConsole` with `enableNativeTerminal()` for the System Terminal and an `InlinePromptConsole` for Alif Output. `Taif` continues to store both pointers as `TConsole*`, preserving the existing `setupUI()` construction path. Prompt-specific lifecycle calls use `qobject_cast<InlinePromptConsole*>`, so no unrelated UI construction was changed.

> **Ownership invariant:** `TConsole` owns the `TerminalSessionController` only when native-terminal mode is enabled. `InlinePromptConsole` never enables that mode and owns only its buffered transcript state.

The System Terminal starts its platform-default interactive shell on the post-show event turn, after the dock owns a real multi-row grid. Windows uses `COMSPEC` with a `cmd.exe` fallback; macOS uses interactive `zsh`; other supported Unix builds use interactive `bash`. The Alif console is created once and accepts inline program input only while `AlifRunController` is in a starting, running, or stopping state.

## Native System Terminal

The native System Terminal has a strict layering boundary that keeps GUI rendering, VT interpretation, and operating-system process transport independently testable.

| Layer | Responsibility | Main types |
|---|---|---|
| Dock-facing host | Preserves the existing Qt dock contract and embeds the viewport in `QPlainTextEdit::viewport()`. | `TConsole` |
| Viewport | Draws the LTR grid, tracks selection and scrollback, encodes keyboard input, and reports debounced grid size changes. | `TerminalView` |
| Session lifecycle | Owns explicit `Idle`, `Starting`, `Running`, `Stopping`, `Finished`, and `Failed` transitions on the GUI thread. | `TerminalSessionController` |
| Transport | Starts, resizes, writes to, and shuts down a platform terminal session. | `ITerminalBackend`, `WindowsConPtyBackend`, `PosixPtyBackend` |
| Rendering model | Maintains cells, attributes, cursor, scroll regions, scrollback, and alternate-screen state. | `TerminalScreenModel` |
| Stream decoder | Incrementally decodes UTF-8 and applies supported terminal controls. | `VtStreamParser` |

On Windows, `WindowsConPtyBackend` uses `CreatePseudoConsole`, `ResizePseudoConsole`, `ClosePseudoConsole`, and a `STARTUPINFOEXW` attribute list carrying `PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE`. It owns Win32 handles with RAII-style cleanup and routes terminal output to the GUI thread through queued Qt signal delivery. Its reader polls available pipe data and exits when the child is complete and output is drained, which prevents one-shot shells from remaining active indefinitely and makes shutdown bounded.

On Linux and macOS, `PosixPtyBackend` creates a native pseudoterminal with `forkpty`, changes into the requested working directory before `execvp`, and reads the master file descriptor from a dedicated thread. Direct input writes to that master; `TIOCSWINSZ` propagates terminal geometry; graceful cancellation signals the child process group before bounded escalation. Both native transports deliver output and completion back to the GUI thread through the same `ITerminalBackend` contract.

> **Terminal lifecycle invariant:** A stop request first sends Ctrl+C while the transport remains active. If the child does not exit within the bounded timeout, the backend terminates it, marks transport inactive, closes pipe handles, joins the reader, and closes the pseudoconsole.

The terminal viewport is intentionally `Qt::LeftToRight`, even when the surrounding Arabic application uses RTL layout. This preserves conventional terminal cell ordering, cursor motion, selection, and escape-sequence fidelity. Its scrollbar spans a single logical row sequence—retained primary-screen scrollback followed by the current grid—so later commands never replace earlier output in the viewport. At the tail it renders the complete active screen; at the top it renders the oldest retained rows. Mouse selection uses that same mapping through the `QAbstractScrollArea` viewport event path.

## VT compatibility contract

`VtStreamParser` is incremental: byte chunks may split a UTF-8 character, escape introducer, CSI parameter list, OSC title sequence, or CRLF pair without resetting parser state. Unsupported sequences are counted rather than causing a parse failure.

| Control family | Supported behavior |
|---|---|
| Text and C0 controls | UTF-8 text, CR, LF, backspace, and tab update the screen model incrementally. |
| Cursor and editing CSI | Relative and absolute cursor moves; display and line erase; character erase/delete; line insert/delete; scrolling; and scroll regions. |
| SGR | Reset, bold, underline, inverse, standard/bright ANSI colors, indexed colors, and RGB foreground/background colors. |
| Private modes | Cursor visibility plus alternate-screen modes `?47`, `?1047`, and `?1049`. |
| OSC | Window title codes `0` and `2`, terminated by BEL or `ESC \\`. |

## Inline Alif Output

`InlinePromptConsole` derives from `TConsole` and retains an editable range only after the prompt marker. Keyboard, mouse, paste, cut, delete, and backspace paths enforce the transcript boundary. Destructive edits initiated in historical output are consumed instead of deleting current input; non-destructive text entry is redirected to the prompt. Submitting with Enter emits the inherited `TConsole::commandEntered(const QString&)` signal, records de-duplicated history, and opens a new prompt. Up and Down traverse that history.

Incoming Alif output is staged in `OutputBuffer` and coalesced by a 16 ms single-shot timer. The GUI thread inserts each drained chunk before the live prompt, preserves the user’s cursor and selection offset in the active input, and follows the bottom only when the user was already following output. Carriage-return state is preserved across chunk boundaries: a split `\r\n` yields one new line, whereas a standalone `\r` replaces the last output line without corrupting the prompt.

| Boundary | Enforced policy |
|---|---|
| Pending output | `OutputBuffer` bounds staged chunks and reports dropped content through one localized truncation notice. |
| Rendered transcript | The document is bounded at 2,000 blocks and 512 Ki UTF-16 characters; old content is incrementally trimmed and identified with a localized notice. |
| Prompt | Only text after the current marker is editable while input is enabled. Output is always inserted before the marker. |
| Alif process lifecycle | `Taif` calls `beginInput()` for active run states and `endInput()` on inactive states and completion. |

`AlifRunController::standardOutput` and `standardError` remain connected to `TConsole::appendPlainTextThreadSafe`, and `TConsole::commandEntered` remains connected to `AlifRunController::sendInput`. The base-class connection stays type-safe because `InlinePromptConsole` is a `TConsole` subclass. Final output draining now checks whether the underlying `QProcess` device remains open, avoiding benign shutdown-time `QIODevice::read` warnings.

## Dock behavior and appearance

`showAndRaiseDock()` and `DockableConsoleToolFactory::showAndActivate()` retain the existing bottom-tab policy. The Alif Output, System Terminal, and Problems docks are re-tabified only when both relevant docks remain non-floating in the bottom area. A deliberate float or move by the user is preserved. The `عرض` menu action state continues to represent each dock’s open state rather than only the selected tab.

Both console surfaces retain the dark navy contract. The terminal grid uses its own direct painter but remains inside the existing dock presentation; the inline Alif transcript remains RTL and uses the existing dark `QPlainTextEdit` palette.

## Validation

The focused UI target compiles all terminal modules and executes native Windows lifecycle coverage, retained-history viewport coverage, and platform-guarded POSIX PTY coverage in addition to existing dock, editor, and console regressions. The production application is separately built from `taif/build/analysis_validation`, then the repository-wide Windows gate is run. Linux and macOS CI runners must execute the same focused target to exercise their local PTY transport.

| Validation command or target | Phase 8 result |
|---|---|
| `TaifDockableToolsTests` | Latest Windows gate: 33 passed, 0 failed. The Unix branch is platform-guarded and requires execution on Linux/macOS runners. |
| Terminal parser/model regression | Passed split CSI, SGR, cursor movement, OSC title, alternate-screen restoration, and retained-history top/tail selection coverage. |
| Terminal session lifecycle regression | Passed a Windows ConPTY interactive command round trip. Linux/macOS use a platform-guarded interactive `/bin/sh -i` PTY round trip. |
| Inline prompt regression | Passed transcript-boundary protection, output-before-prompt insertion, submission, and history traversal coverage. |
| `phase8_app_validate.bat` | Production `Taif.exe` built successfully with all Phase 8 sources. |
| `scripts\\validate_windows.cmd` | Passed application, lexer, parser, semantic, analysis, controller, UI, and hygiene gates. |

## Maintenance rules

Future changes must keep the System Terminal transport independent from `AlifRunController`, must not route terminal input through a separate line editor, and must preserve the `setupUI()` construction boundary. New VT features belong in `TerminalScreenModel` and `VtStreamParser` with parser/model tests; transport changes belong behind `ITerminalBackend`; and Alif transcript behavior belongs in `InlinePromptConsole` with prompt-boundary regressions.
