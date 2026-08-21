# TaifEditor Autosave and Crash Recovery

**Status:** Implemented and validated  
**Applies to:** `RecoveryStore`, `RecoveryCoordinator`, `TRecoveryDialog`, `TEditor`, `Taif`, autosave preferences, and legacy `.~` migration

## Purpose

TaifEditor recovery is a **crash-safety mechanism**, not a replacement for normal file saving. It captures edited document checkpoints in the application-data recovery root and can restore them after an unexpected termination without overwriting user source files.

> **Primary invariant:** A source file is changed only by Taif’s existing `QSaveFile` normal-save path. A recovery snapshot is never committed over a source path automatically.

The implementation keeps the current dark navy visual design and Arabic/RTL-first interaction model. The settings language now describes automatic **recovery snapshots** rather than adjacent backup files.

## Components and Ownership

| Component | Responsibility | Must not do |
|---|---|---|
| `RecoveryStore` | Validates recovery identifiers, writes snapshot and metadata atomically, reads/removes entries, and prunes expired entries. | Construct a file path from a user-controlled path or access editor widgets. |
| `RecoveryCoordinator` | Owns the background writer thread, coalesces writes by document UUID, queues only the newest pending revision, reports write/removal outcomes, and provides signal-driven bounded flush completion. | Retain raw `TEditor*` pointers, pump GUI events, or synchronously sleep while closing. |
| `TEditor` | Tracks dirty, requested, and durably persisted recovery revisions; schedules a 750 ms trailing-idle capture and a maximum-age capture; forms immutable payloads on the GUI thread; and retries failed writes with a bounded delay. | Write recovery data directly to disk or mark a snapshot clean before its acknowledgement. |
| `Taif` | Owns the coordinator, registers editors, defers legacy import until the event loop begins, requests bounded close/discard flushes, and reports failed entry removal. | Change unrelated `setupUI()` construction or block/re-enter the close event loop. |
| `TRecoveryDialog` | Shows recoveries in the existing RTL dark style and returns explicit Restore, Delete, or Defer selection. | Automatically overwrite a user source file. |

## On-Disk Format

Recovery files are stored under:

```text
QStandardPaths::AppLocalDataLocation/recovery/
```

Each entry has a UUID-based snapshot file (`<uuid>.snapshot`) and a metadata file (`<uuid>.json`). Metadata carries a schema version, source path if available, display name, document revision, capture time, source fingerprint, untitled state, and legacy origin.

| Property | Rule |
|---|---|
| Paths | Snapshot and metadata names are generated UUID-based names only. User paths are never used for output filenames. |
| Atomicity | Snapshot writes use `QSaveFile`; metadata is committed only after its snapshot succeeds. |
| Visibility | Missing, malformed, traversing, or incomplete entries are ignored by discovery. |
| Encoding | Recovery text uses UTF-8, matching the existing text-save behavior. |
| Retention | Entries older than 30 days are pruned at startup. |

## Capture Lifecycle

After an editor document changes, `TEditor` increments `currentDirtyRecoveryRevision`, marks recovery dirty, and restarts the single-shot idle timer. The timer captures after 750 ms of typing inactivity. A separate timer enforces the user-configured maximum checkpoint age, currently 5–300 seconds, so a continuously edited document is still protected.

At capture time, the editor prepares an immutable `RecoverySnapshot` containing text, document UUID, monotonic document revision, source path, display name, and current source fingerprint. It records that revision as `lastRequestedRecoveryRevision`, then waits for `RecoveryCoordinator::snapshotPersisted`. The editor updates `lastPersistedRecoveryRevision`, clears the dirty state, and stops capture timers **only** when a successful acknowledgement matches the newest requested revision and no newer edit exists. Per-document coalescing maintains at most one in-flight write and one newest pending snapshot; outdated revisions never replace newer content.

> **Durability invariant:** Enqueuing a recovery write is not persistence. The editor remains recoverably dirty until the matching newest revision is confirmed durable.

A failed matching write leaves the document dirty. It schedules a 30-second retry, bounded to three automatic retries; after that the editor remains dirty so a subsequent edit or explicit close flush can attempt persistence again. A delayed acknowledgement for an earlier request can never clear a later dirty revision.

| Event | Result |
|---|---|
| Document edit | Increments the dirty revision, marks recovery dirty, and schedules capture. |
| Successful snapshot acknowledgement | Advances the persisted revision; only the newest matching acknowledgement clears dirty state and stops capture timers. |
| Failed snapshot acknowledgement | Keeps the document dirty and schedules up to three 30-second retries. |
| Recovery disabled | Stops scheduling but does not delete an already valid recovery snapshot. |
| Successful normal save | Sets the document clean and requests deletion of its recovery entry only after the `QSaveFile` commit. |
| Failed normal save | Leaves the recovery entry intact. |
| Confirmed discard | Requests deletion and reports any asynchronous deletion failure. |
| Canceled close | Leaves the entry intact. |
| Accepted application close | Temporarily disables the window, requests an asynchronous bounded flush, and accepts close only after `flushCompleted(true)`. A deadline or write failure re-enables the window and leaves the user in control. |

## Recovery at Startup

The main window creates `RecoveryCoordinator` before normal launch handling and prunes expired entries. Legacy import and the recovery dialog are deferred with a queued startup callback, so adjacent-backup file I/O no longer runs in the constructor’s GUI path. The callback considers eligible known paths and then presents `TRecoveryDialog` without scanning arbitrary directories.

The dialog uses existing navy surfaces, blue primary action, RTL geometry, and Arabic labels. It lists the recovery label, capture time, and source state.

| Source state | Restore behavior |
|---|---|
| Original source unchanged | Opens the snapshot as a modified editor associated with the source path. The user must still explicitly save. |
| Source changed externally | Opens a separate modified recovery tab without source-path association. It cannot overwrite the changed source automatically. |
| Source missing | Opens a separate modified recovery tab. |
| Untitled document | Opens a modified untitled recovery tab. |
| Deferred | Leaves the entry for a future startup. |
| Deleted | Removes selected metadata and snapshot files. |

Recovery restoration retains the UUID and revision sequence of its entry. A later user Save or explicit Discard removes the correct snapshot. The queued startup recovery callback runs after normal construction, so restoration remains an explicit user action and never silently overwrites an already opened source tab.

## Legacy `.~` Compatibility

New recovery snapshots are **never written beside user files**. For one compatibility release, startup considers only known candidate paths: the optional launch file, recent files, and files stored in named sessions. If an adjacent `.~` file exists and is newer than its source, it is read, atomically imported into the application-data recovery root, and then deleted when possible.

The implementation does not scan arbitrary directories. Unknown legacy `.~` files retain the existing per-file prompt behavior when their source is opened, avoiding broad filesystem searches or surprising destructive cleanup.

## Safety and Maintenance Rules

| Rule | Reason |
|---|---|
| Keep recovery writing out of the GUI thread. | Prevents typing latency and UI stalls on slow storage. |
| Treat `snapshotPersisted` as the durability acknowledgement. | Queue acceptance alone cannot safely clear editor dirty state. |
| Do not use `processEvents()`, nested event loops, or `msleep()` to wait for recovery work. | They re-enter input/close handling and can violate ordering assumptions. |
| Use `requestFlush(deadline)` and `flushCompleted(bool)` for normal close/discard coordination. | The GUI event loop remains responsive and recovery completion has one explicit outcome. |
| Report `removalFailed` rather than silently ignoring an asynchronous deletion error. | The user retains visibility into recoveries that could not be removed. |
| Never remove a recovery snapshot before a successful source `QSaveFile::commit()`. | Avoids data loss when a normal save fails. |
| Do not use an editor pointer in a worker callback. | Editors can close while an asynchronous write is in flight. |
| Restore conflicts as separate modified tabs. | Prevents unapproved overwrite of externally modified files. |
| Preserve explicit completion/analysis revision safety. | Recovery scheduling must not weaken the existing editor pipeline. |
| Keep UUID/file-name validation strict. | Prevents recovery-root traversal and unintended file deletion. |
| Retain the current dark RTL dialog language. | Ensures recovery UI is consistent with TaifEditor’s Arabic-first design. |

## Regression Coverage

The focused UI target validates atomic persistence/read/remove behavior, invalid traversal-style identifier rejection, RTL recovery-dialog defaults, asynchronous non-reentrant flush completion, removal-failure reporting, acknowledged editor persistence, and dirty/retry preservation after a failed write. Full application builds validate qmake/MOC integration with editor scheduling, main-window lifecycle, deferred legacy migration, and startup dialog code.

Before changing recovery behavior, add tests for any new persistence field, worker-state transition, dialog action, or source-conflict rule. Run the full application, lexer, parser, semantic, analysis, and UI suites before delivery. Then remove generated artifacts and run `git diff --check`.
