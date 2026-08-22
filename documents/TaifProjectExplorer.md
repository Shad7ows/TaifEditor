# Taif Project Explorer

**Status:** Implemented as the project-scoped Files tree for TaifEditor’s Qt 6/C++17 desktop IDE.

## Purpose

`ProjectExplorerWidget` replaces the former unfiltered raw `QTreeView` sidebar. It presents exactly one local project rooted at `Taif::folderPath`, retains the application’s dark-navy Arabic RTL shell, and adds safe project file actions, filename/path filtering, persisted local explorer state, provider-based icons, and read-only Git status decorations.

The explorer is a user-interface and intent surface. It does **not** own editor tabs, session records, recovery entries, terminal state, Alif Output, or repository mutation. `Taif` continues to coordinate document lifecycle through its existing file-open/save/close pathways.

## Component boundaries

| Component | Role | Must not do |
|---|---|---|
| `ProjectExplorerWidget` | Renders root header, filter, tree, context menu, empty state, keyboard actions, theme, and persisted UI choices. Emits semantic action requests. | Open tabs directly, mutate files directly, or invoke shell commands. |
| `ProjectFileProxyModel` | Restricts the filesystem source model to a canonical root, filters by relative filename/path, hides temporary/recovery artifacts, and orders folders before files. | Grant access to a path outside the project root. |
| `ProjectFileOperations` | Performs widget-free local create, folder-create, rename, trash/delete, and file-manager reveal operations after root/path validation. | Trust a raw selection/path without validation, rename/delete the root, or build shell commands. |
| `GitStatusService` | Runs a fixed, asynchronous, read-only Git porcelain request and publishes root-relative status values. | Stage, unstage, commit, switch branches, edit Git configuration, or block UI input. |
| Icon/decoration providers | Resolve a base icon and optional state decoration through priority-ordered extension interfaces. | Change explorer filtering, filesystem mutation policy, or editor ownership. |
| `Taif` | Supplies project root, delegates file open, protects open/modified documents, updates tab paths after rename, and reports results. | Reach into Git process internals or bypass operation validation. |

> **Containment invariant:** Every filesystem mutation and reveal request must prove that its canonical/absolute local target is inside the active project root before it occurs. The project root itself can never be renamed or deleted through the explorer.

## Explorer behavior

The root header shows the project name and exposes its full path as a tooltip. The filter searches names and root-relative paths after a small debounce. The outer UI is RTL; technical filesystem strings remain naturally ordered so extensions, separators, and drive prefixes stay readable. Directories remain visible during filtering so a matching descendant remains discoverable when the underlying filesystem model has not yet lazily enumerated it.

The context menu offers **New File**, **New Folder**, **Rename**, **Delete**, and **Open in File Manager**. Actions are disabled for missing, invalid, out-of-root, or project-root selections. Delete first requests a trash move. If that platform operation fails, the user must explicitly confirm irreversible deletion. Rename detects open editors and requires confirmation before the main window updates their file paths, tab titles, and tooltips.

## Version-control decorations

`GitStatusService` executes only the equivalent of:

```text
git status --porcelain=v1 -z --ignored=matching --untracked-files=all
```

It uses an argument vector rather than a shell string, runs from the canonical project root, parses NUL-delimited records, and refreshes through a debounce. If Git is unavailable, the folder is not a repository, or the process fails, the tree remains usable with neutral decoration.

| Git state | Visual treatment | Non-color equivalent |
|---|---|---|
| Modified | Amber/orange filename accent and status dot | Tooltip and accessibility text: `معدّل في Git`. |
| Added/staged | Green accent and dot | `مضاف أو جاهز للإيداع في Git`. |
| Untracked | Cyan accent and dot | `غير متتبّع في Git`. |
| Deleted | Red accent and dot when status is retained | `محذوف في Git`. |
| Renamed | Violet accent and dot | `أُعيدت تسميته في Git`. |
| Conflicted | High-contrast red/magenta dot | `تعارض Git يتطلب المعالجة`. |
| Ignored | Muted treatment when visible | `متجاهل في Git`. |

> **Version-control invariant:** Git state is read-only visual metadata keyed by normalized root-relative path. It must never alter base icon selection, root filtering, filesystem mutation behavior, or active editor ownership.

The status dot, descriptive tooltip, and accessibility description are deliberately combined: a Git state is never conveyed by color alone.

## Icon extension seam

Future in-process plugins can register `IFileIconProvider` or `IFileDecorationProvider` implementations with `ProjectExplorerWidget`. Providers receive a `FileIconContext` containing normalized absolute and root-relative paths, filename/suffix, and directory/symlink state. Higher priority wins; providers can decline a context; the built-in provider always supplies a fallback.

Dynamic plugin discovery and loading are intentionally deferred. A future plugin manager must define plugin lifetime, ABI, permissions, fault isolation, and installation policy before registering third-party providers.

## Persisted state

The explorer stores the active-project-local filter text, hidden-file preference, and last selected relative path through `QSettings`. Persisted values are relative paths and simple scalar preferences, never model indexes, raw Git output, operation history, file content, or credentials.

## Test obligations

Changes must preserve focused regression coverage in `tests/ui/tst_DockableTools.cpp`.

| Area | Required coverage |
|---|---|
| File operations | Invalid names, collisions, out-of-root rejection, root protection, create/rename/delete success and failure. |
| Proxy/model | Canonical root containment, filtering, hidden/artifact policy, and directory-first natural ordering. |
| Explorer UI | RTL shell, LTR-natural technical names, filter debounce, root binding, context action enablement, dark-theme contrast, and empty state. |
| Git service | Added, modified, untracked, deleted, renamed, conflicted, and ignored porcelain mappings; non-repository and unavailable-Git fallback; asynchronous no-shell invocation. |
| Integration | Folder loading, file activation, safe handling of open editors on rename/delete, session/recovery consistency, and no stale root after project changes. |
| Completion gate | Focused UI suite, production build, `scripts\\validate_windows.cmd`, `git diff --check`, and cleanup of temporary artifacts. |

## Deferred source-control work

The next version-control phase may add repository summary, branch display, staged/unstaged workflows, diff views, history/blame, and controlled write commands. Those features must extend the typed status/decoration contracts here rather than reintroducing Git calls inside the tree view or main window.
