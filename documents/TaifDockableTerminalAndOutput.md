# Dockable Alif Output and System Terminal

**Status:** Implemented and validated  
**Applies to:** Qt 6 / C++17 TaifEditor main window, tool docks, terminal console, and Alif process output

## Purpose

TaifEditor now exposes the two existing console experiences as independent, persistent Qt tool docks:

| Dock title | Object name | Console object name | Purpose |
|---|---|---|---|
| `المشكلات` | `DiagnosticsDock` | `DiagnosticsPanel` | Current-document errors and warnings. |
| `مخرجات ألف` | `AlifOutputDock` | `AlifOutputConsole` | Alif execution output, errors, completion status, and interactive stdin. |
| `طرفية النظام (CMD/Bash/Zsh)` | `TerminalDock` | `SystemTerminalConsole` | Persistent platform shell terminal. |

All three are real `QDockWidget` tools in the bottom dock area. They are movable, floatable, closable/hideable, and initially tabified together. Users can drag them into a different dock area or float them through normal Qt docking behavior.

## Ownership and lifecycle

`DockableConsoleToolFactory` is the single construction boundary for terminal/output tool widgets. It creates the dock, applies the standard allowed areas/features, creates a child `TConsole`, applies RTL console configuration, installs the console as the dock widget, and adds the dock to the bottom of the supplied `QMainWindow`.

> **Persistence invariant:** Hiding or closing a console dock changes visibility only. It does not recreate its `TConsole`, erase buffered text, terminate the system shell, or duplicate process signal connections.

The system terminal is created once during `Taif::setupUI()` and begins its platform shell session immediately. The Alif output console is also created once, but its dock remains hidden until `runAlif()` begins. Output history survives later runs unless the run logic explicitly clears it at the start of the next execution, as it does today.

## Execution-output behavior

`runAlif()` no longer searches a splitter-hosted tab collection or manipulates central splitter geometry. It uses the persistent `alifOutputConsole`, clears/updates it for the next program run, and calls `showAndRaiseDock(alifOutputDock)`.

The managed execution connections remain intentionally narrow:

| Signal or action | Destination | Behavior preserved |
|---|---|---|
| `AlifRunController::standardOutput` | `TConsole::appendPlainTextThreadSafe` | Standard output is streamed to `مخرجات ألف`. |
| `AlifRunController::standardError` | `TConsole::appendPlainTextThreadSafe` | Standard error is streamed to `مخرجات ألف`. |
| `TConsole::commandEntered` | `AlifRunController::sendInput` | Program input entered in the output dock is delivered to the active managed Alif process. |
| `AlifRunController::finished` | Output console and run-controller state transition | Exit outcome is rendered and active-run controls are restored exactly once. |

The system terminal is intentionally independent of `AlifRunController`; showing, hiding, floating, or tabifying the output dock never affects the native shell process.

## Output buffering and rendering policy

Both console docks use the shared `OutputBuffer` contract before rendering through `TConsole`. Producers may append decoded `QString` chunks from any thread. The GUI consumer drains one bounded batch on demand and appends text blocks to the existing `QTextDocument`; it does not rebuild the entire console text with `setPlainText()` on a fixed timer.

| Boundary | Policy |
|---|---|
| Pending output | At most 1 MiB of UTF-16 chunk data is staged. Individual chunks are capped at 256 KiB. When a burst exceeds either bound, oldest pending content is discarded and a localized truncation notice is appended. |
| Rendered output | The document is capped at 2,000 blocks and 512 Ki UTF-16 characters. Oldest rendered text is trimmed incrementally; a localized notice marks rendered-history trimming. |
| Flush cadence | The single-shot 16 ms timer is dormant while idle and coalesces only active bursts. It performs append/trim work rather than a full-document reset. |
| CR/LF handling | `\r`, `\n`, and CRLF pairs are statefully interpreted across chunk boundaries. A standalone carriage return overwrites the current line, while a split CRLF still creates exactly one new line. |
| Encoding | The interactive Windows shell uses the local Windows code page for child output/input. Unix shell paths use UTF-8. `AlifRunController` continues to own the Alif execution encoding contract before text reaches the output console. |
| Failure reporting | Terminal `QProcess::errorOccurred` appends a localized diagnostic to the same terminal surface. |

> **Lifecycle invariant:** Hiding a dock never stops its console. Destruction invokes the existing bounded terminal terminate/kill policy, drains already queued text once, and cannot leave a repeating idle flush timer active.

## Dock visibility and selected-tab policy

The existing F6 console action now toggles `TerminalDock`. If the dock is hidden **or is an inactive tab**, it is selected in the shared bottom tab group and focused. It is hidden only when it is already the rendered terminal tab; focus then returns to the active editor. Running a Taif file always selects `AlifOutputDock` beside `المشكلات`.

`DockableConsoleToolFactory::isRenderedTab()` uses the dock’s rendered visible region rather than `isVisible()` alone, because an inactive dock in a Qt tab group can remain logically visible. This gives F6 a reliable distinction between “select the terminal tab” and “hide the currently selected terminal.”

`showAndRaiseDock()` is the main-window policy boundary. When it receives either console dock, it first asks `DockableConsoleToolFactory::ensureTabifiedWith()` to restore that dock to the Problems tab group **only while both docks remain non-floating in the bottom area**. It then calls `showAndActivate()`, which shows the dock, raises it immediately, and schedules one deferred raise after Qt completes pending dock-layout work. The deferred activation is necessary because `show()` alone makes a dock logically visible without necessarily selecting its tab.

> **Selected-tab invariant:** Starting Alif makes `مخرجات ألف` the rendered tab in the bottom group. Opening the system terminal makes `طرفية النظام` the rendered tab in that same group. The Problems dock remains a peer tab, not a displaced dock or a separate splitter surface.

A dock that the user deliberately floats or moves to a different dock area is not forcibly re-tabified. This preserves intentional workspace customization while guaranteeing the expected shared tab behavior for the standard bottom-tool layout.

The splitter now contains only the editor tabs and the pre-existing search surface.

## View-menu commands

`TMenuBar` now provides the Arabic **`عرض`** menu, which is a command-only surface: it emits semantic requests to `Taif` and never owns or manipulates dock widgets directly.

| Menu item | Action object name | Target | Command behavior |
|---|---|---|---|
| `مخرجات ألف` | `ShowAlifOutputAction` | `AlifOutputDock` | Shows and selects the Alif output tab. |
| `الطرفية` | `ShowTerminalAction` | `TerminalDock` | Shows and selects the persistent system-terminal tab without recreating the shell. |
| `الأخطاء` | `ShowProblemsAction` | `DiagnosticsDock` | Shows and selects the Problems tab without modifying its diagnostics or filters. |

The three actions are independently checkable. `Taif::syncBottomToolActionState()` maps every dock’s open state to its matching action after menu activation, F6 terminal handling, Alif execution activation, and dock visibility changes. A dock being open is sufficient to earn a check mark, so multiple tools can be checked simultaneously even when Qt renders only one member of their tab group. Hiding or closing a dock clears only that dock’s check mark.

## Visual and RTL policy

Terminal and Alif output docks reuse the Problems dock’s dark-blue visual contract:

- body background `#0f172a`;
- title surface `#1e293b`;
- right-aligned Arabic-compatible title styling;
- shared muted border and transparent close/float controls;
- RTL `TConsole` input/output configuration.

Future docks should reuse these selectors or a later shared dock-theme service rather than introducing independent tool-window styles.

## Validation

The focused UI regression target `tests/ui/ui_tests.pro` validates the reusable dock factory without starting a real system shell. It also validates bounded pending output, truncation accounting, split CRLF behavior, carriage-return overwrite behavior, append-only rendering, rendered line/character limits, and preservation of the latest output during a large burst.

| Validation target | Result |
|---|---|
| `TaifDockableToolsTests` widget and console regression | Phase 6: 28 passed, 0 failed. |
| Full TaifEditor Qt 6.11.1 / MSVC 2022 application build | Passed. |
| Existing lexer, parser, semantic, and analysis suites | Must remain green in the full phase validation matrix. |

## Future extension

The stable dock object names prepare the main window for a future `WorkspaceSession` implementation using `QMainWindow::saveState()` and `restoreState()`. That future work can persist user dock placement, tabification, floating state, and visibility without changing the console ownership contract.

A later `DockManager` may centralize tool registration, titles, shortcuts, visibility commands, theme tokens, and layout persistence. This implementation deliberately limits itself to terminal/output migration and does not move the search panel or redesign the general main-window command architecture.
