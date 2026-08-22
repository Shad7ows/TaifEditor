# TaifEditor Information Bar

**Status:** Implemented as an editor-grade bottom information bar for Qt 6/C++17.

## Purpose

`EditorInfoBar` replaces the former single line-and-column label with a compact persistent presentation of the active editor’s state. It is hosted as a permanent widget in `QStatusBar`; standard transient main-window messages continue to use the status bar’s ordinary message area.

The information bar is intentionally separate from the Problems dock, breadcrumb bar, terminal docks, and Alif Output. It summarizes their relevant editor state without taking ownership of diagnostics, semantic analysis, file persistence, or process execution.

## Architecture

| Layer | Responsibility | Primary type |
|---|---|---|
| Editor state producer | Own document, cursor, diagnostics, analysis, and recovery services; build an immutable presentation snapshot. | `TEditor` |
| State contract | Transport typed editor state without exposing internal service objects to the main window. | `EditorInfoSnapshot` |
| Presenter | Format state into themed RTL segments, define tooltips/accessibility, compact on narrow widths, and emit user actions. | `EditorInfoBar` |
| Main-window coordinator | Bind only the current editor, prevent stale-tab updates, and route diagnostics activation to the existing Problems dock. | `Taif` |

> **Ownership invariant:** `Taif` must not access `EditorAnalysisBinding` or `EditorRecoveryBinding` directly. New state required by the information bar belongs in `EditorInfoSnapshot` and is exposed by a narrow `TEditor` façade.

## Snapshot contract

`EditorInfoSnapshot` carries document metadata, modification state, cursor and selection coordinates, document size, UTF-8/EOL/indentation metadata, diagnostic totals, semantic-analysis metrics, and recovery persistence state. It is widget-free and has no presentation strings beyond neutral technical tokens.

`TEditor::informationSnapshot()` produces a complete snapshot. `editorInformationChanged(EditorInfoSnapshot)` is emitted after cursor movement, document changes, modification transitions, diagnostic changes, semantic revision/metric updates, recovery transitions, and preference application. Main-window bindings always verify that the signal still belongs to `currentEditor()` before applying it.

| State source | Bar segment | Display policy |
|---|---|---|
| File path and document modified state | Document | Filename plus a modified marker; full path is a tooltip. |
| Cursor and selection | Cursor and selection | Cursor is always visible; selection replaces ordinary document-size summary when non-empty. |
| Document | Selection/document | Compact line and character count, with expanded tooltip. |
| Diagnostics | Problems | Error/warning count; click activates the existing Problems dock. |
| Semantic analysis | Analysis | Pending, ready with duration, or large-document state; detailed size/token metrics remain in tooltip. |
| Recovery binding | Recovery | Saved, pending persistence, or retry-scheduled status. |
| Preferences and source metadata | Format | UTF-8, EOL convention, and indentation width/mode. |

## Directionality and visual design

The outer bar uses `Qt::RightToLeft` so its segment order matches the Arabic application. Technical labels within each segment use `Qt::LeftToRight`; line/column values, UTF-8, LF/CRLF, indentation width, timing, and numeric punctuation therefore remain conventional and unambiguous.

The presenter preserves Taif’s dark-navy design with low-contrast separators, readable light text, blue interactive diagnostics, red error emphasis, and amber warning/pending emphasis. Every compact segment exposes an accessible name or tooltip containing its expanded meaning.

> **Directionality invariant:** Never make technical text RTL merely because the containing UI is RTL. The bar’s shell is Arabic RTL; technical coordinates and protocol-style tokens remain LTR.

## Responsive priority

The bar must not produce a horizontal scrollbar or force the editor/dock geometry to expand. It keeps core state visible first and reduces secondary detail as width becomes constrained.

| Width band | Visible segments |
|---|---|
| Standard | Document, diagnostics, analysis, recovery, selection/document, cursor, and format. |
| Dense | Document, diagnostics, selection/document, cursor, and format; analysis/recovery compact away. |
| Compact | Diagnostics, cursor, and format remain visible; document and selection detail compact away. |

Tooltips preserve the full state for a segment that has been visually compacted.

## Main-window lifecycle

`Taif::onCurrentTabChanged()` binds the information bar to the new active editor, refreshes one complete snapshot, and disconnects the preceding editor connection. The empty state is applied if no editor is active. File-open, save, recovery, preferences, and cursor operations are reflected through normal editor signals; no polling timer or direct service inspection is permitted.

The diagnostics segment is the only interactive status-bar segment in this version. Its activation calls `showAndRaiseDock(diagnosticsDock)`, preserving existing bottom-dock tabification and visibility policy.

## Test obligations

Changes to this subsystem must update `tests/ui/tst_DockableTools.cpp` with focused coverage.

| Change area | Required regression coverage |
|---|---|
| Snapshot fields | Cursor, selection, document size, modified state, format, diagnostics, analysis, and recovery values. |
| Presentation | RTL outer layout, LTR technical labels, dark-theme segment text, tooltips, and accessible controls. |
| Responsive behavior | Secondary segments compact at narrow widths while diagnostics, cursor, and format remain visible. |
| Interaction | Diagnostics activation emits once and routes through the Problems-dock path. |
| Editor lifecycle | Active-editor changes refresh the bar and never retain stale previous-tab data. |
| Completion gate | Focused UI target, production build, `scripts\\validate_windows.cmd`, `git diff --check`, and artifact cleanup. |

## Maintenance rules

Future contributors must not reintroduce individual cursor labels in `Taif`, place analysis/recovery formatting inside `TEditor`, make technical values RTL, or make the bar depend on a periodic polling timer. Add a status-bar preference only when it can preserve the snapshot ownership boundary and the responsive priority rules above.
