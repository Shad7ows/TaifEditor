# Taif Git Panel

**Status:** Implemented as the right-side Git workflow surface for TaifEditor’s Qt 6/C++17 IDE.

## Purpose and ownership

The Git panel gives a local single-root project a complete everyday/team source-control surface without moving repository process logic into the file tree or main window. The **Project Explorer** continues to own filesystem presentation and receives lightweight decorations. `GitRepositoryService` is the single shared source of truth for repository state; `GitPanelWidget` presents that state and emits user intent; `Taif` owns dock visibility and protects editor lifecycle around destructive or workspace-changing operations.

| Component | Owns | Must not own |
|---|---|---|
| `GitRepositoryService` | Asynchronous repository snapshots, porcelain parsing, fixed-vector Git execution, diff/history text, process lifetime, refresh coalescing. | Widgets, tabs, credentials, or shell commands. |
| `GitPanelWidget` | Dark RTL panel presentation, grouped changes, commit composer, sync/branch controls, diff/history/output views. | Constructing Git command strings or silently modifying editors. |
| `ProjectExplorerWidget` | File tree, file operations, shared Git decorations, manual filesystem/Git refresh button. | A second Git status cache or Git mutation commands. |
| `Taif` | Right-side dock/action, current project root, confirmation, dirty-editor protection, and user-facing lifecycle coordination. | Git parsing/process management. |

> **Single-source-of-truth invariant:** The Git panel and the Project Explorer always read the same `GitRepositorySnapshot`. A manual refresh re-roots the asynchronous filesystem model, requests a non-debounced Git refresh, replays refresh requests that arrive during an active query, and repaints tree decorations when the snapshot changes.

## Git panel behavior

The right-edge **Git** button toggles the dock. The panel is RTL for Arabic controls and prose, while technical panes—branch/ref names, hashes, Git output, remote/upstream identifiers, file paths, and unified diffs—remain LTR/naturally ordered.

| Panel area | Behavior |
|---|---|
| Repository summary | Project/repository name, current branch, upstream, ahead/behind counts, clean/dirty/error state, and manual refresh. |
| Change groups | Conflict, staged, ordinary changed, and untracked paths; each has a semantic state and selected-file diff. |
| Daily actions | Stage, unstage, file-scoped discard, commit message and commit. |
| Team actions | Fetch, fast-forward-only pull, push, local branch create, and existing-branch switch. |
| History | Bounded recent commit list with hash, subject, author, date, and refs tooltip. |
| Diagnostics | Copyable LTR command output/error area; clear Arabic user result text. |

Project Explorer decorations retain the state colors and non-color equivalents: status dot, tooltip, and accessible description. Clean/unavailable state remains neutral.

## Command safety

Every operation starts `git` with a fixed program and argument vector through `QProcess`. No command is built through a shell string. Repository-relative file paths, branch names, and remote names pass typed validation before launch. Git commands are serialized; a command cannot begin while another command is running. Query/command output is capped before display and never interpreted as instruction.

| Operation | Git operation | Safety behavior |
|---|---|---|
| Refresh | `status --porcelain=v1 -z -b --ignored=matching --untracked-files=all`, then `remote`. | Asynchronous/coalesced; no-repository/Git-unavailable fallback; stale refresh replay. |
| Stage / unstage | `add -- <paths>` / `restore --staged -- <paths>`. | Valid local repository-relative paths only. |
| Discard | `restore --worktree -- <paths>`. | Explicit irreversible confirmation; blocked if an affected file is open. |
| Commit | `commit -m <message>`. | Requires non-empty message and at least one staged file. |
| Fetch | `fetch <remote>`. | Requires a validated configured remote. |
| Pull | `pull --ff-only`. | Requires upstream; dirty/modified editor safety checks; merge/non-fast-forward stops with Git guidance. |
| Push | `push`. | Requires an upstream; Git/credential error is shown without credential interception. |
| Branch create/switch | `switch -c <branch>` / `switch <branch>`. | Ref-name validation and dirty-editor/worktree confirmation. |
| Diff/history | Bounded `diff --no-ext-diff` and bounded formatted `log`. | Read-only, cancellable by service shutdown, technical LTR result display. |

> **Credential and conflict invariant:** Authentication prompts, credential-manager interaction, MFA, host verification, and conflict resolution remain under user control. The panel reports failures and preserves Git’s resulting state; it does not capture secrets or auto-resolve conflicts.

## Editor and lifecycle safety

Before discard, Taif refuses to run if an affected path is currently open. Before pull or branch switching, it refuses when an editor buffer is modified and requests confirmation when the worktree is dirty. This prevents a Git operation from silently replacing content that exists only in an unsaved editor buffer.

After a successful mutation, the shared service refreshes status. Project Explorer receives the new snapshot and repaints its colors/dots automatically. Operations can be cancelled during service destruction; shutdown sets a guard that prevents query callbacks from starting a replacement process.

## Test obligations

| Area | Required coverage |
|---|---|
| Repository service | Normal/non-repository discovery, NUL porcelain mapping, Arabic/space paths, availability fallback, status refresh replay, root change during query, process shutdown. |
| Command contract | Validation rejection, stage/unstage/discard/commit result, output bounds, serialized execution, no shell construction. |
| Panel | Right-side toggle state, RTL shell/LTR technical widgets, clean/empty/busy/error rendering, grouped changes, diff/history, action enabled states. |
| Lifecycle | Dirty/open editor blocks discard/pull/switch; refresh updates file-tree decorations; docks/actions stay synchronized. |
| Completion | Focused UI tests, production build, `scripts\validate_windows.cmd`, `git diff --check`, and no temporary logs. |

## Deferred work

Repository initialization/cloning, remote management, first-push upstream setup, stash, tags, merge/rebase orchestration, an in-editor conflict resolver, reset/revert, branch deletion/merge, signing, credentials, Git LFS, submodules, and multi-root workspaces remain deliberately deferred. Future work must extend `GitRepositoryService` typed contracts and maintain the shared-snapshot invariant.
