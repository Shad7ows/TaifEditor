# TaifEditor Local AI Assistant

## Purpose and operating model

The **AI Assistant** is TaifEditor’s dockable, left-side local coding workspace. It connects to a user-managed model hosted by LM Studio through the OpenAI-compatible HTTP API and provides streaming chat, explicit context attachment, controlled project tools, and a concise engineering activity view. TaifEditor does not install, launch, or manage LM Studio; the user loads a compatible model and starts its server before refreshing the model list.

The default operating model is **Workspace Auto**. It permits the agent to complete a bounded sequence of validated project-local reads, searches, and narrowly allowlisted local build/test/read-only Git commands without interrupting the user for every low-risk step. Existing-file patches are deliberately **staged for visual review** rather than written automatically. It is intentionally not unrestricted shell or filesystem authority. A hard safety stop always takes precedence over autonomy.

| Setting | Default | Behavior |
|---|---:|---|
| Endpoint | `http://127.0.0.1:1234/v1` | LM Studio’s local OpenAI-compatible base URL. |
| Model timeout | 10 minutes | A streaming **inactivity** deadline, renewed whenever a valid SSE choice arrives. The stored range is 30 seconds to 1 hour. |
| Command timeout | 5 minutes | A bounded project-local command deadline. The stored range is 5 seconds to 30 minutes. |
| Autonomy | Workspace Auto | Validated low-risk workspace operations may continue automatically; Manual mode requires confirmation for every operation. |
| Maximum response | 200,000 characters | Bounds streamed model output. |
| File context | 30,000 characters | Bounds a deliberately attached active document or selection. |
| Tool output | 40,000 characters | Bounds read/search/command data retained for model continuation. |

The AI panel is opened from **عرض → مساعد الذكاء الاصطناعي**. It is initially left-docked, can be moved, floated, or closed, and follows TaifEditor’s RTL dark-navy design. Arabic prose follows RTL layout while endpoints, model IDs, paths, code, and commands remain LTR where technically appropriate.

## Conversation, context, and presentation

The composer sends only the written prompt unless the user explicitly elects to attach the active selection or active document. Changing an editor, selection, or active tab refreshes available context but does not itself transmit source. The assistant’s streamed prose appears in the chat transcript; raw tool output and source read from the workspace do **not** appear there.

Instead, each completed tool step is represented by a short activity and a source-free transcript marker. The activity list reports the action, its affected file or outcome, and test/command status. Double-clicking an activity opens a compact summary dialog. That dialog is intentionally bounded to metadata and result summaries; it does not render raw source or raw tool payloads. Approval cards likewise describe intended impact without embedding a full file replacement or command payload in the main chat surface.

## Workspace Auto execution policy

The agent can take at most **12 autonomous continuation turns** for one user request. It also tracks repeated tool signatures; after the same request reaches the repeat threshold, it pauses for review. Stop cancels an active model request or command and prevents further automatic continuation.

| Tool | Workspace Auto behavior | Mandatory stop conditions |
|---|---|---|
| `list_project_tree` | Runs automatically after root containment is established. | Missing project root or malformed call. |
| `read_project_file` | Runs automatically for bounded UTF-8 text beneath the root. | Outside root, binary content, unreadable or oversized file. |
| `search_workspace` | Runs automatically over bounded textual project files. | Empty/malformed search or missing root. |
| `get_active_editor_context` | Runs automatically only for already available in-memory editor context. | No active context available. |
| `propose_file_patch` | Creates a staged side-by-side review after path, file size, text, and SHA-256 snapshot checks succeed. | Explicit Accept is always required; outside root, missing/binary/oversized file, stale snapshot, repeated request, or any matching unsaved open editor fail closed. |
| `propose_create_file` | Runs automatically only for a new contained text path. | Existing target, outside root, malformed request, or repeated request. |
| `propose_rename_path` | Never automatic. | Always requires review. |
| `propose_delete_path` | Never automatic. | Always requires review; the underlying operation targets the OS trash. |
| `propose_terminal_command` | Runs automatically only when it passes the narrow command policy. | Any unrecognized, composite, destructive, network, package, shell, process, credential, or elevation-related command. |

The command policy fails closed. Its automatic allowlist is deliberately limited to plain `qmake`, `nmake`/`make`, local `cmake --build`, local `ctest`, and read-only `git status`, `git diff`, `git log`, or `git branch` commands. It rejects shell wrappers, redirection, pipes, chaining, expansions, package managers, interpreters, Node/Python execution, installs, publish/push/reset/clean Git actions, `clean`, install/package targets, and every command not explicitly recognized. Processes launch directly with a program and argument list rather than through `cmd.exe` or `/bin/sh`; completion is one-shot and reports a distinct timeout result when the agent terminates a command for exceeding its deadline.

## Hard workspace safeguards

Every workspace path is normalized and checked with `ProjectFileOperations::normalizedPath()` and `isInsideRoot()` before it is read, searched, changed, renamed, deleted, or emitted as a mutation notification. Text reads/searches reject binary and oversized input. Patch requests capture a SHA-256 snapshot before execution and write through `QSaveFile`, refusing the update if the target changed meanwhile. Creation refuses to overwrite an existing path.

TaifEditor tracks **all** open modified editor files, not only the active tab. Every existing-file patch opens a central VS Code–style split review **as soon as the model streams a complete target path**. The current file and progressively emitted proposed revision appear in read-only LTR source panes with removed lines highlighted on the original side and added lines highlighted on the proposed side. While content is still streaming, the workspace identifies itself as a live preview and both Accept and Reject are disabled; it cannot write a file. Once the tool call finishes parsing, the same surface becomes the final staged review and enables the Arabic RTL controls. The file remains unchanged until **قبول التعديل** is selected. Rejection never writes the file and gives the model a concise tool result so it can adapt. Multiple proposed patches are reviewed in deterministic tool-call order; continuation pauses until each staged review is resolved.

On acceptance, TaifEditor repeats root, text/binary/size, SHA-256 snapshot, and all-open-unsaved-editor checks before using `QSaveFile` for an atomic commit. A stale source or unsaved matching editor therefore still fails closed even after a proposal is visible. When a safe mutation succeeds, the main window refreshes Project Explorer and Git state, refreshes diagnostics and breadcrumbs, and reloads only a matching editor that is still clean. An editor with unsaved content is never silently reloaded or replaced.

Non-loopback endpoints remain subject to the existing privacy acknowledgement requirement. Endpoint/control initialization blocks widget signals so loading saved Workspace Auto mode or timeout values cannot accidentally persist a partially loaded setting. Saving a newly entered remote endpoint is refused unless the required acknowledgement is already available.

## Multi-step tool continuation

For tool-capable LM Studio models, an assistant tool batch is preserved in the conversation history using OpenAI-compatible assistant `tool_calls` records. Each subsequent `role: tool` result carries its matching `tool_call_id`. This ordering is retained before Workspace Auto starts the next model turn, allowing the model to reason over the result and choose the next safe action. Invalid or rejected calls still receive a concise tool result so the model can adapt; pending approvals pause continuation until the review is resolved.

A normal assistant response ends the autonomous task. A connection error, the step budget, repeat protection, an unresolved approval, or explicit Stop also ends or pauses the task. The model is instructed never to claim a file or command completed unless a corresponding tool result confirms it.

## Lifecycle and compatibility

The panel explicitly owns and shuts down its `AiAgentController`. Shutdown aborts transport, stops the command deadline, and kills an active agent process. Hiding the dock preserves the in-memory conversation; **مسح المحادثة** clears it. The assistant does not replace or alter Alif Output, System Terminal, Project Explorer/Git, diagnostics, recovery/autosave, language analysis, minimap, folds, completion, or multi-cursor behavior.

## Validation obligations

| Area | Required regression check |
|---|---|
| Settings | Workspace Auto defaults, 10-minute model timeout, 5-minute command timeout, and bounded normalization are verified. |
| UI | The AI dock remains left-docked/RTL and discoverable from **عرض**; Workspace Auto and timeout controls are present; raw tool payloads are absent from the transcript. |
| Transport | Model discovery, SSE streaming, cancellation, inactivity renewal, malformed data, response limits, and assistant `tool_calls` serialization are deterministic. |
| Continuation | Assistant tool-call metadata precedes matching tool results; automatic safe results advance one bounded model turn; rejection gives the model an adaptation result. |
| Command safety | The direct-launch command policy accepts only the narrow allowlist and requires approval for uncertain, shell, destructive, network, package, interpreter, or process operations. |
| Filesystem safety | Root containment, staged patch-without-write behavior, acceptance-only atomic save, stale patch rejection, no-overwrite create, all-open-unsaved-editor protection, and mutation refresh behavior are covered. |
| Patch review UI | The split panes are read-only/LTR, change highlights and dark-theme accept/reject controls are visible, normal tabs restore after resolution, and raw patch source remains absent from the chat transcript. |
| Compatibility | The focused UI suite, production Windows build, repository Windows validation gate, and `git diff --check` remain green. |

## Operational limits

Workspace Auto is local and intentionally conservative. It does not persist a transcript, start LM Studio, install dependencies, access credentials, elevate privileges, launch background agents, perform browser/network automation, silently delete/rename paths, or write an existing-file patch without the explicit staged-review acceptance action. New tool classes or command allowances must be introduced through a testable fail-closed policy, retain the project-root boundary, and add explicit regression coverage.
