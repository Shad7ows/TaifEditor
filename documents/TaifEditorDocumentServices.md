# TaifEditor Document Services

**Status:** Implemented and validated  
**Applies to:** `TEditor`, `EditorAnalysisBinding`, `EditorRecoveryBinding`, `EditorInteractionBinding`, `EditorAnalysisController`, recovery snapshots, semantic hover, and editor preferences.

## Purpose

`TEditor` remains the **widget façade** for the existing RTL code-editing experience. It continues to own visual construction, editor events, line numbers, minimap geometry, completion popup presentation, hover popup geometry, folding, and all public editor APIs consumed by `Taif`.

Document-level scheduling and timer state are now isolated in three non-visual QObject services. This reduces constructor concentration without changing the existing dark navy design, Arabic labels, `هذا` semantics, completion acceptance behavior, or the established three-tier analysis timing.

> **Facade invariant:** Services may coordinate stable document signals and immutable payloads, but they must not change widget layout, style, text direction, completion ordering, or directly access widgets from worker-thread callbacks.

## Service Boundaries

| Service | Owns | TEditor retains | Shutdown rule |
|---|---|---|---|
| `EditorAnalysisBinding` | `QTextDocument::contentsChange` to `EditorAnalysisController`, semantic source snapshot capture, analysis snapshot metrics, and controller shutdown. | Fast-pass painting, diagnostics presentation, semantic completion refresh, breadcrumbs, and visual highlighter updates. | Disconnect document/controller links, then call the controller’s idempotent `shutdown()`. |
| `EditorRecoveryBinding` | Autosave/max-age/retry timers, revision state, payload capture, durable-write acknowledgement, bounded retry policy, and recovery metrics. | Source-path metadata and text payload factory; public recovery façade methods. | Stop timers, disconnect coordinator/document links, and discard callback references before QObject child destruction. |
| `EditorInteractionBinding` | Hover debounce timer and revision-associated pending pointer state. | Hit testing, semantic hover/definition resolution, popup styling/geometry, Ctrl-hover underline, completion, and navigation history. | Stop timer and clear pending state idempotently. |

All three services are Qt children of `TEditor`. `TEditor::~TEditor()` calls shutdown in this order: **recovery → interaction → analysis**. Each service also calls its own idempotent shutdown from its destructor as a defensive fallback.

## Preserved Analysis Pipeline

The scheduling contract remains unchanged.

| Tier | Timing | Owner | Preserved behavior |
|---|---:|---|---|
| Tier 0 | 0 ms | `EditorAnalysisController` | Revision increment and dirty-range coalescing from `QTextDocument::contentsChange`. |
| Tier 1 | 150 ms | `EditorAnalysisController` | Fast lexical/highlighter request for the current revision. |
| Tier 2 | 300 ms | `EditorAnalysisBinding` + controller | GUI-thread immutable text capture followed by background lexer/parser/semantic analysis; stale revisions are discarded. |

`EditorAnalysisBinding` supplies the source snapshot only when the controller requests the current Tier 2 revision. It measures capture duration and forwards the unchanged immutable source text to the existing controller. Analysis timing remains available through the existing `LanguageAnalysisSnapshot::metrics` values.

## Recovery Contract

`EditorRecoveryBinding` retains the acknowledged-persistence policy introduced in Phase 2. It tracks dirty, requested, and persisted revisions; a document becomes clean only after the newest requested snapshot is acknowledged successfully. Failed writes keep the document dirty and schedule at most three automatic 30-second retries.

| Measured value | Meaning |
|---|---|
| Recovery payload characters | UTF-16 character count captured for the snapshot payload. |
| Recovery payload capture duration | GUI-thread time spent constructing the immutable recovery payload. |
| Recovery write duration | Elapsed time from submission to the matching durable success/failure acknowledgement. |
| Analysis source characters | Character count of the Tier 2 immutable analysis source. |
| Analysis source capture duration | GUI-thread time spent producing that source snapshot. |
| Analysis metrics | Existing lexer/parser/semantic/presentation/total durations and token/span counts from `LanguageAnalysisSnapshot`. |

## Performance Boundary

The analysis binding classifies source snapshots at or above **512 Ki UTF-16 characters** as a large document and emits that condition with its metric signal. This is an observability threshold only: it does **not** alter language semantics, lexical highlighting, parser behavior, or the 0/150/300 ms scheduling policy.

Future phases should consume these signals for non-disruptive telemetry and progressive handling. Search-match volume and console backlog remain separate concerns; they must not be solved by introducing unbounded synchronous work into editor event handlers.

## Interaction Contract

The interaction binding controls only the hover delay and pending token state. It preserves the current preference-controlled hover delay. `TEditor` still resolves semantic information from the revision-matched snapshot and preserves the current left-of-pointer, dark-navy RTL hover popup placement.

Completion, dot-member lookup, `هذا` handling, constructor-field visibility, popup dismissal, and go-to-definition remain in `TEditor` for now because they are tightly coupled to key events and existing UI acceptance semantics. Any future extraction must keep completion acceptance from deleting the typed dot and must not reopen a dismissed/accepted popup.

## Regression Coverage

The focused UI suite validates recovery acknowledgement/retry behavior and now constructs, initializes, and shuts down all three document services twice to prove idempotent teardown. Full application and language suites verify production qmake/MOC wiring and preserve lexer/parser/semantic/analysis behavior.

Before changing service boundaries, add regression coverage for the relevant destruction order, revision gate, source snapshot metric, and user-facing interaction behavior. Run the full application build, lexer, parser, semantic, analysis, and UI suites. Remove generated qmake files, release/debug output, and temporary validation scripts before delivery.
