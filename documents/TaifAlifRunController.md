# TaifEditor Alif Run Lifecycle

**Status:** Implemented and validated  
**Applies to:** `source/run/AlifRunController.*`, `Taif::runAlif()`, the existing Run menu/toolbar actions, and the Alif Output dock.

## Purpose

`AlifRunController` replaces the previous ad-hoc `ProcessWorker`/`QThread` execution path. It owns exactly one non-interactive Alif compiler/runtime process and exposes an explicit lifecycle suitable for the editor’s existing output dock.

> **Primary invariant:** A run request either reaches one explicit terminal outcome—normal completion, abnormal completion, or launch failure—or is explicitly canceled. A previous execution is never silently deleted or replaced by a new worker object.

The controller preserves the current Arabic/RTL interface and dark navy visual language. Existing Run controls remain in place. While a process is active, the Run action changes to **إيقاف التنفيذ** and invoking it requests cancellation.

## Ownership and State

`Taif` owns one `AlifRunController` as a Qt child. The controller owns one `QProcess`, configured with separated standard-output and standard-error channels. It does not own editor widgets or console widgets.

| State | Meaning | Allowed next state |
|---|---|---|
| `Idle` | No active process has been requested. | `Starting` |
| `Starting` | A validated request was given to `QProcess::start()`. | `Running`, `Failed`, `Finished` |
| `Running` | The process emitted `started`. | `Stopping`, `Finished`, `Failed` |
| `Stopping` | Cancellation or teardown requested graceful termination. | `Finished`, `Failed` |
| `Finished` | The process reached a normal exit. | `Starting` |
| `Failed` | Launch failed or the process crashed/encountered a terminal error. | `Starting` |

A new start request is rejected while the controller is in `Starting`, `Running`, or `Stopping`. The rejection returns an Arabic error message and leaves the existing process untouched.

## Request Validation and Launch

A request contains a program path, arguments, working directory, and display name. The controller validates that the executable exists as a file and that the requested working directory exists before calling `QProcess::start()`.

`Taif::runAlif()` retains the existing save-before-run rule. It requests a normal editor save when the active document has no path or has modifications. It then resolves the packaged Alif executable beneath the application directory and starts the managed controller with the source file as its argument and source directory as working directory.

The old synchronous version probe (`alif -ن` plus `waitForFinished`) was intentionally removed. The regular execution path is fully signal-driven and does not block the editor UI before launch.

## Output, Failure, and Cancellation

| Event | Controller behavior | Main-window behavior |
|---|---|---|
| Standard output | Emits `standardOutput(QString)`. | Appends text to the existing Alif Output console. |
| Standard error | Emits `standardError(QString)`. | Appends text to the existing Alif Output console. |
| Failed launch | Emits `launchFailed(QString)` and changes to `Failed`. | Shows a localized explanation in the output console. |
| Normal completion | Flushes pending channels, changes to `Finished`, emits exit code/status exactly once. | Adds the existing completion separator and exit-code message. |
| Abnormal completion | Flushes pending channels, changes to `Failed`, emits completion exactly once. | Adds an abnormal-completion message. |
| User cancellation | Calls `terminate()` and enters `Stopping`. | The output dock records that stopping was requested. |
| Unresponsive cancellation | Escalates to `kill()` after 1.2 seconds. | Completion is still reported through the same terminal path. |
| Main-window teardown | Gracefully terminates, then kills if needed within a 1.5-second bounded shutdown. | No process/thread callback may outlive the controller. |

The controller uses local system encoding on Windows and UTF-8 elsewhere, matching the project’s existing console policy. If this policy is changed, both interactive terminal and Alif controller decoding must be changed together and tested with Arabic compiler output.

## Maintenance Rules

| Rule | Reason |
|---|---|
| Do not recreate or delete the process while it is active. | Avoids lost output, duplicate completion callbacks, and unowned process lifetime. |
| Do not use `waitForStarted()` or a synchronous compiler-version probe in the regular Run action. | Keeps opening/running responsive. |
| Keep all `QProcess` use in the controller’s GUI-thread lifetime. | Output is delivered directly to UI-owned console buffering without cross-thread widget access. |
| Keep cancellation idempotent. | Menu/toolbar actions and teardown can safely request stop more than once. |
| Preserve exactly-once completion delivery. | Prevents duplicate output separators and inconsistent action state. |
| Reserve blocking waits for bounded destructor shutdown only. | Normal interactions must remain event-driven. |
| Keep source save semantics unchanged. | Execution must never run an unsaved document version silently. |

## Regression Coverage

The focused UI suite verifies that invalid launch requests are rejected, a real short process streams output and completes, and a longer process can be canceled to a terminal outcome. The production application build validates qmake/MOC wiring and the main-window migration from raw workers to the controller.

Before changing execution behavior, add tests for every state transition, error path, cancellation timeout, output ordering, and teardown scenario. Run the full application and language/UI regression matrix before delivery, then remove generated test artifacts and run `git diff --check`.
