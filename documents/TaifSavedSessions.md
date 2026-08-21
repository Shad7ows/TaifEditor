# Saved Sessions and Session Restore

**Status:** Implemented and validated  
**Applies to:** `SessionStore`, `SessionEditorDialog`, `WelcomeWindow`, `ApplicationWindowController`, and `Taif`

## Purpose

TaifEditor supports explicit named sessions from the welcome page. A session is a durable, user-managed list of files that opens into one editor window as ordered tabs. It is deliberately distinct from automatic workspace recovery: session mutations are explicit, and unsaved buffer contents are never persisted.

Phase 5 keeps the existing Arabic/RTL workflow and user-visible behavior while replacing destructive whole-group `QSettings` rewriting with a **versioned atomic session repository**. A failed write cannot silently turn a previously valid session list into an empty one.

> **Persistence boundary:** A session stores identities of on-disk files, never unsaved in-memory editor text, backups, terminal state, or UI layout.

## Session Contract

| Field | Meaning | Invariant |
|---|---|---|
| `id` | Opaque UUID-like stable identity | Never shown as UI text; CRUD is ID-based. |
| `displayName` | Human-readable Arabic or other user-provided name | Trimmed; case-folded duplicate names are rejected. |
| `filePaths` | Ordered list of cleaned absolute paths | Duplicate paths are normalized out while preserving first occurrence order. Missing paths remain stored. |
| `activeFilePath` | Last intended active file for the explicit session definition | Selected after restore only if it is successfully opened. |
| `updatedAt` | UTC update timestamp | Refreshed on create and update. |

## Atomic Repository Contract

The production repository is a versioned JSON document named `saved-sessions.json` under the application configuration directory. Isolated tests use a sibling `*.sessions.json` path derived from the supplied test INI scope. `SessionStore` is still the sole persistence boundary; Welcome and dialog code do not serialize JSON or access raw settings keys.

| Item | Contract |
|---|---|
| Primary repository | JSON root with a schema version and a `sessions` array. Each entry contains the session contract fields. |
| Atomic commit | `QSaveFile` writes the complete payload and commits one replacement. Open, write, or commit failure leaves the previous primary intact. |
| Last-good backup | Before replacing an existing primary, it is copied to `saved-sessions.json.bak`. Backup-copy failure aborts the mutation rather than risking the only good copy. |
| Read validation | The reader validates root version/type, entry types, IDs, unique case-folded names, normalized file paths, and active-file containment. |
| Recovery | A malformed or unsupported primary falls back to the last-good backup. If neither is valid, `loadAll()` reports a localized error; it does not pretend the store is empty. |
| Logical serialization | `create`, `update`, `remove`, and `saveAll` use one process-local recursive mutex across their read-modify-write sequence to prevent logical lost updates from competing UI actions. |

## Legacy QSettings Migration

Previous releases stored sessions in the `SavedSessions` QSettings group. When neither the JSON primary nor backup exists, `SessionStore` reads those legacy entries defensively, preserving the prior tolerant behavior of skipping malformed or duplicate records. It returns valid sessions and attempts an atomic JSON migration.

The legacy QSettings group is **not deleted**. This retains a compatibility fallback while migration stabilizes. If migration fails, the valid legacy sessions remain available to the caller and a localized migration error is returned; no stored session is destroyed.

| Situation | Load result |
|---|---|
| Valid JSON primary | Return validated primary sessions. |
| Corrupt primary, valid backup | Return validated backup sessions. |
| No JSON files, valid legacy QSettings | Return sessions and attempt atomic migration without deleting legacy data. |
| No stored sessions | Return an empty list without error. |
| Invalid primary and invalid/missing backup | Return no sessions with a localized error. |

## Welcome-Page Workflows

| Interaction | Behavior |
|---|---|
| `جلسة جديدة` | Opens the RTL session editor. The user chooses a name and any number of files, then saves an explicit session. Empty sessions are allowed for later organization. |
| Session list double-click | Opens the selected session. The list displays a friendly name and file count; its stable ID is stored in item data only. |
| `إدارة الجلسات` | Opens a dark RTL management dialog with open, edit, delete, and create actions. |
| Edit session | Changes the display name and ordered file list. Add, remove, move-up, and move-down operate only on the pending definition until Save. |
| Delete session | Requires Arabic confirmation and deletes only the stored session definition—never user source files. |
| No sessions | Shows the welcome-page empty state instead of a static non-functional session placeholder. |

The editor dialog visibly marks unavailable paths as `ملف غير موجود`; these remain editable and removable rather than being silently discarded.

## Safe Editor Restore

`Taif::restoreSession()` is intentionally distinct from interactive `openFile()`.

| Restore concern | Implementation rule |
|---|---|
| Per-file save prompts | Never call interactive `openFile()` in a restore loop. `openDocumentFile()` loads session files noninteractively. |
| Duplicate paths | Canonical normalized paths are loaded once, in saved order. |
| Default untitled tab | Welcome creates the target window without a placeholder. If no files can be restored, the restore path creates exactly one valid untitled editor. |
| File load state | Each restored tab receives the same diagnostics, editor-action state, open-request, tab tooltip, and recent-file integration as an interactively opened file. |
| Backup recovery | The per-file backup prompt is intentionally disabled during a bulk restore to avoid a modal prompt storm. |
| Active file | The saved active path is selected if loaded; otherwise the first restored file is selected. |
| Missing/unreadable files | They are skipped but retained in storage. The restore result returns all unavailable paths, and Welcome shows one aggregated Arabic warning only after the editor window is visible. |

> **Safety invariant:** Opening a session does not write, delete, rename, move, or silently repair user source files.

## UI and RTL Requirements

Session editor and management UI are right-to-left, use the established navy/slate Taif surfaces, preserve Arabic labels, and use tooltips for complete file paths. `SessionEditorDialog` supplies stable object names for test and accessibility inspection:

| Control | Object name |
|---|---|
| Name input | `SessionNameInput` |
| Pending files | `SessionFilesList` |
| Add files | `AddSessionFilesButton` |
| Remove file | `RemoveSessionFileButton` |
| Move file up/down | `MoveSessionFileUpButton` / `MoveSessionFileDownButton` |

## Regression Coverage

The focused UI target validates normalized create/update/remove behavior, legacy QSettings migration, primary corruption fallback to backup, ordered RTL session editing, and draft-independent session restore behavior. It runs beside the existing dock, View menu, Edit menu, floating search, recovery, run-controller, and document-service checks.

Production validation must always also include lexer, parser, semantic, analysis, UI, and full application build targets.

## Maintenance Rules

Do not put persistence logic in `WelcomeWindow`; keep serialization, validation, path normalization, migration, backup, and name policy inside `SessionStore`. Do not use a displayed session name as identity. Do not replace `restoreSession()` with a loop over `openFile()`, because that reintroduces per-file save prompts and focus churn. Do not remove missing entries automatically: the user must be able to repair or delete them intentionally through session management. Any future automatic workspace persistence should be separate and must not silently overwrite these explicit saved sessions.
