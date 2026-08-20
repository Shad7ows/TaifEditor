# RTL Edit Menu and Find/Replace Design

**Status:** Implemented and validated  
**Applies to:** `TMenuBar`, `Taif`, `SearchPanel`, and the active `TEditor` document

## Purpose

TaifEditor exposes Arabic editing commands through the RTL **`تحرير`** menu. The menu is the command surface only: `TMenuBar` owns persistent `QAction` objects and semantic signals, while `Taif` resolves those requests against the active editor. No menu object accesses editor or dock state directly.

| Command group | Menu commands |
|---|---|
| History | `تراجع`, `إعادة` |
| Clipboard | `قص`, `نسخ`, `لصق` |
| Find and navigation | `بحث`, `بحث واستبدال`, `الذهاب إلى سطر` |
| Line editing | `تعليق سطر`, `تكرار السطر`, `نقل السطر لأعلى`, `نقل السطر لأسفل` |

## Command routing

| Action | Shortcut | Handler | Behavior |
|---|---|---|---|
| `تراجع` | Standard Undo | `TEditor::undo()` | Uses Qt’s native undo stack. |
| `إعادة` | Standard Redo | `TEditor::redo()` | Uses Qt’s native redo stack. |
| `قص`, `نسخ`, `لصق` | Standard Cut/Copy/Paste | `TEditor` inherited clipboard slots | Preserves native selection, read-only, and clipboard semantics. |
| `بحث` | Standard Find | `Taif::showFindBar()` | Opens the compact RTL find row. |
| `بحث واستبدال` | `Ctrl+H` | `Taif::showReplaceBar()` | Opens the find row plus replacement controls. |
| `الذهاب إلى سطر` | `Ctrl+G` | `Taif::goToLine()` | Moves to a requested line in the active editor. |
| `تعليق سطر` | `Ctrl+/` | `TEditor::toggleComment()` | Comments or uncomments the current selection/line. |
| `تكرار السطر` | `Ctrl+D` | `TEditor::duplicateLine()` | Duplicates the current line. |
| `نقل السطر لأعلى/لأسفل` | `Alt+Up` / `Alt+Down` | `TEditor::moveLineUp()` / `moveLineDown()` | Moves the current line without changing the central editor layout. |

> **Shortcut invariant:** A command shortcut is registered on its persistent menu action. Do not add a parallel `QShortcut` for the same command in `Taif`, or one key press may invoke the command twice.

## RTL find and replace

`SearchPanel` is a persistent, frameless floating overlay owned by `Taif`, not a member of the editor splitter. `showIn(activeEditor)` anchors its rounded dark-blue surface at the upper RTL edge of the active editor, tracks editor resize/move/show events, and keeps it above the editor without consuming document space. It has a find row and an optional replacement row; exposes search text, replacement text, case-sensitive, whole-word, and regular-expression options; navigation controls; replacement controls; a match counter; and a no-match visual state. The panel remains RTL and returns focus to the active editor when closed.

`Taif` owns search/replace execution because it already owns the active tab/editor relationship. `SearchReplaceEngine` owns the pure match collection and replacement transaction so the same C++17 implementation can be directly regression-tested without constructing the main window. `Taif` applies amber highlights, selects the current match, and reports `current/total` to the panel.

| Operation | Safety rule |
|---|---|
| Find next/previous | Wraps through the current document and updates the selected match plus highlight count. |
| Whole-word search | Treats letters, numbers, and `_` as word characters. |
| Regex search | Rejects invalid patterns without changing document text. |
| Replace one | Replaces the selected/containing match, then refreshes the match selection. |
| Replace all | `SearchReplaceEngine` uses one `QTextCursor` edit block and replaces from end to start so offsets remain valid. One Undo restores the entire operation. |
| Read-only editor | Replace commands are disabled and have no effect. |

## Edit-action state

`Taif::updateEditActionState()` synchronizes menu enablement from the active editor, selection, undo/redo stack, document read-only state, and clipboard text. It is refreshed when tabs change, editor selection/cursor and text state change, undo/redo availability changes, and clipboard contents change.

| State | Expected availability |
|---|---|
| No active editor | All Edit commands are disabled. |
| Editable editor without selection | Undo/redo are availability-dependent; Cut/Copy are disabled; Find/navigation and line actions are enabled. |
| Text selected | Copy is enabled; Cut is enabled only when the editor is writable. |
| Clipboard contains text and editor is writable | Paste is enabled. |
| Read-only editor | Copy and Find/navigation remain available; mutating commands are disabled. |

## Regression coverage

`tests/ui/tst_DockableTools.cpp` now covers the Edit menu’s RTL direction, ordered actions, shortcuts, stable action object names, and semantic signal routing. It also covers the SearchPanel’s floating parent/anchor contract, frameless presentation, upper-edge placement, resize repositioning, replace-row lifecycle, find/replace command signals, search/replacement text contract, match counter, and no-match styling. Direct `SearchReplaceEngine` coverage verifies whole-word filtering, regex validation safety, end-to-start replacement behavior, and that one Undo restores a complete Replace All operation.

The wider lexer, parser, semantic, analysis, and dock/view menu regression suites remain mandatory before shipping a change to this area.

## Maintenance rules

Do not bypass `TMenuBar` signals by having a menu action call editor methods directly. Keep advanced search logic in `SearchReplaceEngine` and active-editor UI orchestration in `Taif`, not in the visual `SearchPanel`. Do not place the search panel back in the editor splitter: it must remain a persistent floating child of the main window and be positioned only through `SearchPanel::showIn()` / its anchor-event handling. Preserve the end-to-start replacement direction and one-edit-block Replace All transaction. Any future search highlights must compose safely with existing editor extra selections rather than accidentally removing unrelated semantic decorations.
