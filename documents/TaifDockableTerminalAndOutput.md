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

The existing worker connections remain intact:

| Signal or action | Destination | Behavior preserved |
|---|---|---|
| `ProcessWorker::outputReady` | `TConsole::appendPlainTextThreadSafe` | Standard output is streamed to `مخرجات ألف`. |
| `ProcessWorker::errorReady` | `TConsole::appendPlainTextThreadSafe` | Standard error is streamed to `مخرجات ألف`. |
| `TConsole::commandEntered` | `ProcessWorker::sendInput` | Program input entered in the output dock is delivered to the active Alif process. |
| `ProcessWorker::finished` | Output console and worker thread cleanup | Exit status is printed and the worker thread is stopped. |

The system terminal is intentionally independent of `ProcessWorker`; showing, hiding, floating, or tabifying the output dock never affects the native shell process.

## Dock visibility and selected-tab policy

The existing F6 console action now toggles `TerminalDock`. If the dock is hidden **or is an inactive tab**, it is selected in the shared bottom tab group and focused. It is hidden only when it is already the rendered terminal tab; focus then returns to the active editor. Running a Taif file always selects `AlifOutputDock` beside `المشكلات`.

`DockableConsoleToolFactory::isRenderedTab()` uses the dock’s rendered visible region rather than `isVisible()` alone, because an inactive dock in a Qt tab group can remain logically visible. This gives F6 a reliable distinction between “select the terminal tab” and “hide the currently selected terminal.”

`showAndRaiseDock()` is the main-window policy boundary. When it receives either console dock, it first asks `DockableConsoleToolFactory::ensureTabifiedWith()` to restore that dock to the Problems tab group **only while both docks remain non-floating in the bottom area**. It then calls `showAndActivate()`, which shows the dock, raises it immediately, and schedules one deferred raise after Qt completes pending dock-layout work. The deferred activation is necessary because `show()` alone makes a dock logically visible without necessarily selecting its tab.

> **Selected-tab invariant:** Starting Alif makes `مخرجات ألف` the rendered tab in the bottom group. Opening the system terminal makes `طرفية النظام` the rendered tab in that same group. The Problems dock remains a peer tab, not a displaced dock or a separate splitter surface.

A dock that the user deliberately floats or moves to a different dock area is not forcibly re-tabified. This preserves intentional workspace customization while guaranteeing the expected shared tab behavior for the standard bottom-tool layout.

The splitter now contains only the editor tabs and the pre-existing search surface.

## Visual and RTL policy

Terminal and Alif output docks reuse the Problems dock’s dark-blue visual contract:

- body background `#0f172a`;
- title surface `#1e293b`;
- right-aligned Arabic-compatible title styling;
- shared muted border and transparent close/float controls;
- RTL `TConsole` input/output configuration.

Future docks should reuse these selectors or a later shared dock-theme service rather than introducing independent tool-window styles.

## Validation

A dedicated UI regression target, `tests/ui/ui_tests.pro`, validates the reusable dock factory without starting a real system shell. It confirms bottom-area registration, standard dock features, widget ownership, stable object names, tabification with Problems, widget persistence after hide/show, and rendered-tab selection after activating Alif output and the system terminal.

| Validation target | Result |
|---|---|
| `TaifDockableToolsTests` widget regression | Passed: 4 tests, including selected-tab activation for output and terminal. |
| Full TaifEditor Qt 6.11.1 / MSVC 2022 application build | Passed. |
| Existing lexer, parser, semantic, and analysis suites | Previously green and unaffected; this change does not modify language/editor-analysis code. |

## Future extension

The stable dock object names prepare the main window for a future `WorkspaceSession` implementation using `QMainWindow::saveState()` and `restoreState()`. That future work can persist user dock placement, tabification, floating state, and visibility without changing the console ownership contract.

A later `DockManager` may centralize tool registration, titles, shortcuts, visibility commands, theme tokens, and layout persistence. This implementation deliberately limits itself to terminal/output migration and does not move the search panel or redesign the general main-window command architecture.
