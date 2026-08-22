# TaifEditor Multi-Cursor Editing

## Purpose and scope

TaifEditor supports **professional, bounded multi-cursor editing** in `TEditor` while retaining the native `QPlainTextEdit` cursor as the **primary cursor**. The feature is intended for repeated technical and Arabic RTL edits without changing the established analysis, recovery, syntax, diagnostics, minimap, fold, completion, navigation, or docked-tool architecture.

> The primary cursor is authoritative. It is always retained as an edit target, even when a secondary cursor overlaps or sorts before it. Secondary cursors are auxiliary state and are never allowed to replace primary-cursor semantics.

The implementation is deliberately conservative. It supports predictable text transactions and clears secondary state before existing complex single-cursor workflows take ownership. This policy prefers a correct single-cursor edit over an ambiguous multi-cursor mutation.

## User interaction contract

| Action | Shortcut or gesture | Result |
|---|---|---|
| Add or remove a secondary caret | `Alt+Click` | Toggles a point caret at the clicked document position. It cannot be placed inside the primary selection. |
| Add a vertical caret above | `Alt+Shift+Up` | Toggles a caret in the visible preceding block at the same logical column, clamped to that block’s length. `Ctrl+Alt+Up` remains a compatibility alias. |
| Add a vertical caret below | `Alt+Shift+Down` | Toggles a caret in the visible following block at the same logical column, clamped to that block’s length. `Ctrl+Alt+Down` remains a compatibility alias. |
| Select next occurrence | `Ctrl+Alt+D` | Adds the next unselected, non-overlapping occurrence of the primary selection, wrapping once through the document. |
| Select all occurrences | `Ctrl+Shift+L` | Adds every non-overlapping occurrence of the primary selection. |
| Clear secondary carets | `Esc` | Removes every secondary caret and selection. |
| Duplicate the current line | `Ctrl+D` | Remains the established duplicate-line command. It is not repurposed by multi-cursor editing. |

Occurrence commands require a non-empty, single-line primary selection. Matching text is literal and document-order based. A selection spanning a paragraph is intentionally not treated as an occurrence request.

## Architecture

`MultiCursorController` in `source/texteditor/interaction` owns only the secondary `QTextCursor` values and document-level edit primitives. `TEditor` owns the controller, remains the only owner of the native visible primary cursor, translates UI events into controller operations, and refreshes presentation state.

| Component | Responsibility | Invariant |
|---|---|---|
| `TEditor` | Primary cursor, shortcut routing, event lifecycle, completion/IME safety, selection overlays, direct caret painting, and existing editor integration. | The editor’s `textCursor()` is always the primary cursor. |
| `MultiCursorController` | Secondary state, cap enforcement, cursor normalization, occurrence selection, vertical placement, and one-block text transactions. | No transaction can exclude the primary cursor. |
| `QTextDocument` | Storage, native undo stack, change notifications, analysis/recovery triggers, and RTL text semantics. | Each supported multi-edit is one `beginEditBlock()` / `endEditBlock()` undo operation. |

### Cursor normalization

The controller stores at most **500 secondary cursors**. Before every edit, it builds a new snapshot with the primary cursor first, then appends only valid secondaries that do not duplicate, overlap, or fall inside a retained cursor selection. The primary cursor is therefore always preserved irrespective of physical document order.

Operations are applied from higher document positions toward lower positions. This descending order preserves source offsets for each pending mutation. `QTextCursor` tracks subsequent lower-position mutations, so the returned primary cursor is the post-transaction primary location.

## Supported editing behavior

The following operations are applied to the primary cursor and every normalized secondary cursor in a single undoable document transaction.

| Operation | Behavior |
|---|---|
| Ordinary text input | Replaces each selection or inserts at each caret. Arabic and RTL text use the same native `QTextCursor` character offsets as every other document edit. |
| Backspace | Removes the active selection, or the character immediately before each caret. |
| Delete | Removes the active selection, or the character immediately after each caret. |
| Tab | Inserts a tab at every normalized cursor. |
| Return / Enter | Inserts a newline at every normalized cursor. Leading spaces and tabs are preserved per source line; if the preceding non-whitespace character is `:`, one tab is added, matching the established single-cursor `curserIndentation()` behavior. |

Selection fill overlays are produced through `highlightCurrentLine()` and use the existing dark-navy visual language. Secondary carets are drawn directly in `paintSecondaryCursors()` after the native editor paint, with a high-contrast blue marker. Presentation refreshes occur when carets are added, removed, edited, or cleared; painting itself never changes selection state.

## Conservative fallback matrix

Some editor paths have custom state machines that cannot be safely duplicated without a deeper feature-specific design. When those paths are invoked while secondary cursors exist, TaifEditor clears the secondaries and routes the original event through the proven primary-cursor implementation.

| Interaction | Multi-cursor policy | Reason |
|---|---|---|
| Completion popup acceptance and navigation | Clear secondary cursors, preserve existing completion behavior. | Completion replacement ranges and popup ownership are primary-cursor specific. |
| Snippet insertion and placeholder navigation | Clear secondary cursors, preserve existing snippet behavior. | Snippet target state is primary-cursor specific. |
| Auto-pairing and quote/bracket skipping | Clear secondary cursors, preserve existing pairing behavior. | Pair insertion and skip-over semantics require structural per-caret policy. |
| Undo, redo, navigation, selection-extension, and other unsupported key routes | Clear secondary cursors, then use native editor behavior. | Native cursor navigation must not leave stale secondary coordinates. |
| IME input, drag/drop, context-menu mutation, and external document edits | Clear secondary state on the normal document change path. | These mutations are not represented by controller transactions. |
| Focus loss and ordinary single-cursor mouse actions | Clear secondary cursors. | Prevents dormant carets from surviving a context change. |
| Ctrl+Click definition navigation | Remains first in mouse routing and is unchanged. | Definition navigation must retain its established priority. |

A controller-initiated document transaction sets a scoped transaction guard in `TEditor`. This prevents the synchronous `QTextDocument::contentsChange` lifecycle callback from clearing secondary state in the middle of a valid multi-edit. All non-controller document changes clear secondary state immediately.

## RTL and folded-block considerations

Caret coordinates use logical `QTextCursor` positions, not screen-direction arithmetic. This preserves Qt’s document behavior for Arabic RTL source while allowing mixed technical punctuation and Latin identifiers. Vertical caret placement retains the logical `positionInBlock()` and clamps it to the destination block length. A target in a hidden folded block is rejected; no caret is silently placed into an invisible region.

## Regression obligations

Every change to this subsystem must retain focused coverage in `tests/ui/tst_DockableTools.cpp`, followed by the project’s normal Windows validation gate.

| Regression area | Required assertion |
|---|---|
| Primary invariant | A secondary overlapping the primary selection is rejected; a transaction edits the primary even when document order differs. |
| Deduplication | Repeating the same `Alt+Click`-equivalent toggle removes the secondary caret. |
| Occurrences | `Ctrl+Alt+D` selects the next match; `Ctrl+Shift+L` selects all matches; Arabic text replacement remains atomic. |
| Vertical carets | `Alt+Shift+Down` produces same-column logical edits and honors boundaries; the Ctrl+Alt compatibility alias remains covered by the editor routing. |
| Editing | Text replacement, backspace, delete, tab, and indentation-aware newline are each one undo step. |
| Lifecycle | `Esc`, focus changes, and unsupported routes clear secondaries rather than corrupting text. |
| Visual integration | An added secondary caret changes the rendered editor image; selection overlays refresh with cursor state. |
| Compatibility | `Ctrl+D` remains duplicate line, and existing completion, snippet, auto-pairing, definition, diagnostics, recovery, minimap, and fold paths remain validated by the broader suite. |

## Validation commands

The focused suite runs headlessly on Windows with the Qt platform plugin configured:

```powershell
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_QPA_PLATFORM_PLUGIN_PATH = 'C:\Qt\6.11.1\msvc2022_64\plugins\platforms'
.\release\TaifDockableToolsTests.exe
```

The final change gate is `scripts\validate_windows.cmd`. It must pass after a production build and the focused test suite. Build logs and transient result files are not retained in the repository.
