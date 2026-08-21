# TaifEditor Startup and Main-Window Lifecycle

**Status:** Implemented and validated  
**Applies to:** `ApplicationBootstrap`, `ApplicationWindowController`, `main.cpp`, `TSettings`, `WelcomeWindow`, and non-visual lifecycle code in `Taif`

## Purpose

TaifEditor startup is intentionally split into three layers. `ApplicationBootstrap` owns process-wide application policy that must exist before any UI is created; `ApplicationWindowController` owns top-level Welcome/editor routing and transition lifetime; and `main.cpp` only constructs `QApplication`, invokes bootstrap, validates the optional launch file, delegates initial routing, and enters the event loop. This prevents resource-font registration, Arabic/RTL defaults, application metadata, command-line validation, and window ownership policy from being duplicated across windows.

`Taif` remains the main-window coordinator. Its existing `setupUI()` implementation is explicitly preserved: its layouts, widget hierarchy, docks, toolbar, object names, insertion order, Arabic labels, and visual behavior are not part of this lifecycle refactor.

## Startup Contract

| Startup responsibility | Owner | Invariant |
|---|---|---|
| `QApplication` construction | `main.cpp` | Bootstrap runs only after a valid application instance exists. |
| Application identity | `ApplicationBootstrap` | Organization is `Alif` and application is `Taif` before any `QSettings` consumer is constructed. |
| Arabic-first direction | `ApplicationBootstrap` | The application default is `Qt::RightToLeft`; widgets may override it only deliberately. |
| Resource-font registration | `ApplicationBootstrap` | Fonts are registered independently; one failure never disables other valid fonts. |
| Font family access | Named catalog accessors | UI code never reads `QFontDatabase` using a registration index. |
| Global scrollbar QSS | `ApplicationBootstrap` | Cross-window scrollbar policy is applied once. |
| Optional launch file | `ApplicationBootstrap::parseLaunchRequest()` | Zero or one positional file is valid; more than one returns an Arabic error message. |
| Initial window route | `ApplicationWindowController` | A launch file opens `Taif`; no file opens `WelcomeWindow`. |
| Top-level window creation | `ApplicationWindowController` | `main.cpp`, `WelcomeWindow`, and `Taif` do not construct one another directly. |
| Welcome actions | `WelcomeWindow` | Emits new-file, file, folder, or session intent only; it has no `Taif` construction or restore policy. |
| Return to welcome | `Taif` + `ApplicationWindowController` | `Taif` emits a request; the controller waits for accepted destruction before creating/showing Welcome. A rejected close cancels the pending transition. |

> **Ordering rule:** No `Taif`, `WelcomeWindow`, or `TSettings` instance may be constructed before `ApplicationBootstrap::initialize()` completes. `ApplicationWindowController` is created only after launch validation succeeds.

## Top-Level Window Ownership

`ApplicationWindowController` is the sole process-level owner of top-level routing. It creates a `WelcomeWindow` lazily, tracks every created `Taif` instance with Qt lifetime signals, and centralizes new-file, open-file, open-folder, and saved-session restoration. The controller preserves existing Arabic dialogs and recovery behavior because it delegates actual editor/session work to the existing `Taif` APIs.

| Transition | Controller behavior |
|---|---|
| Startup with no path | Creates/shows `WelcomeWindow`. |
| Startup/open with a path | Creates/shows `Taif` with that path. |
| Welcome new/file/folder intent | Creates the appropriate editor window, shows/activates it, then closes the welcome surface. |
| Welcome session intent | Creates an editor with no default document, restores the session, then reports unavailable files in the existing Arabic dialog. |
| Taif “exit to welcome” intent | Marks only that editor as pending return, requests normal close, and creates/shows Welcome only after destruction. |
| Cancelled editor close | Clears the pending return state; no stray Welcome window appears later. |

> **Ownership invariant:** UI windows express intent through signals. Only `ApplicationWindowController` decides which top-level window to create, show, close, or re-activate.

## Font Catalog

The font catalog exposes semantic roles rather than registration IDs. `uiArabicFamily()` and `displayArabicFamily()` provide Arabic-facing family choices, `codeMonospaceFamily()` provides the code-facing choice, and `availableEditorFontFamilies()` provides the stable de-duplicated list shown by settings.

| Consumer | Required access pattern | Prohibited pattern |
|---|---|---|
| Application default font | Use `availableEditorFontFamilies()`. | Assume a fixed registration sequence. |
| Settings font picker | Use `availableEditorFontFamilies()`. | Loop over `applicationFontFamilies(0..N)`. |
| Welcome-window title | Use `displayArabicFamily()`. | Read a hard-coded application-font ID. |
| Future code widgets | Use `codeMonospaceFamily()` or an explicit user setting. | Copy a raw resource path or positional ID. |

Catalog accessors have system-family defaults so a reduced test target or an unavailable resource cannot cause empty-family indexing or a startup crash.

## `Taif` Lifecycle Rules

### Preserved UI Boundary

`Taif::setupUI()` is a protected compatibility boundary for this refactor. Do not split, reorder, extract, rename, or otherwise modify it merely to improve construction aesthetics. Non-visual helpers may be added outside that method, but all UI construction must remain in the established implementation unless the user separately approves a UI refactor.

### One-Time Connections

The settings widget is created with `Taif` as its QObject parent. Font-size, font-family, and highlighter-theme signals are connected once through `connectSettingsSignals()` during construction. `openSettings()` only shows, raises, and activates the existing settings window; it must never attach another signal connection.

The active editor’s cursor-position callback is managed through `cursorPositionConnection`. A tab change disconnects the prior callback before connecting the selected editor. This mirrors breadcrumb connection handling and prevents repeated tab switching from multiplying status-bar updates.

| Connection category | Lifetime | Rule |
|---|---|---|
| Settings signals | Main-window lifetime | Connect exactly once. |
| Active cursor position | Active-tab lifetime | Store and disconnect before rebinding. |
| Breadcrumb context | Active-tab lifetime | Store and disconnect before rebinding. |
| Editor diagnostics and editor action state | Individual editor lifetime | Attach once when the editor is created. |

## Save and Close Safety

Document-close decisions are typed through `SaveDecision` instead of integer sentinel values. Every save prompt receives the specific `TEditor*` under consideration, so closing a non-active tab cannot prompt or save the active tab by mistake.

| Situation | Required behavior |
|---|---|
| Closing an unmodified tab | Close immediately. |
| Closing a modified tab | Prompt for that exact tab; save, discard, or cancel. |
| App close | Evaluate every open editor in tab order; stop immediately on cancel. |
| Menu exit to welcome | Request normal close; create the welcome window only after every editor accepts close preparation. |
| Saving an existing file | Write through `QSaveFile`, then commit atomically. |
| Saving an untitled file | Ask for a path, then use the same atomic writer. |
| Atomic write failure | Leave the editor modified and show an Arabic error; do not finalize tab/title/breadcrumb state. |
| Successful save | Update file path, clear modified state, remove backup, update tab/window state, and refresh active breadcrumbs. |

> **Correctness rule:** Never call a save-decision method that implicitly looks up `currentEditor()` when the operation is targeting an editor from a tab index or an app-close loop.

## Regression Requirements

| Target | Required coverage |
|---|---|
| Focused UI suite | Bootstrap sets application identity and RTL direction; named font roles are non-empty; valid and invalid launch requests are distinguished; the lifecycle controller routes Welcome → editor → Welcome; and prior dock, menu, search, session, and breadcrumb tests remain green. |
| Full application build | Registers `ApplicationBootstrap` and `ApplicationWindowController`, resolves all lifecycle/font-consumer includes, and links `Taif` save/close changes. |
| Language suites | Lexer, parser, semantic, and analysis behavior must remain unchanged. |

Before accepting future startup or lifecycle changes, build the full application and run all regression targets. Remove generated qmake files, release/debug outputs, and temporary validation scripts before final delivery.

## Recommended Follow-On

Managed Alif execution is now owned by `AlifRunController`. The next isolated hardening phase should instead reduce `TEditor` constructor and recovery/presentation responsibility concentration without combining editor-cohesion work with lifecycle or `setupUI()` changes without a separate approved plan.
