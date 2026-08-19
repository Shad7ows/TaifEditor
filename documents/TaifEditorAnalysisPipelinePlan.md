# Taif Editor Three-Tier Analysis Pipeline

**Status:** Baseline 0 ms, 150 ms, and 300 ms tiers implemented and validated with the dedicated analysis test target. Incremental language-core reanalysis remains planned.  
**Scope:** Per-editor Qt 6/C++17 scheduling that connects the existing highlighter with the Taif lexer, parser, and semantic model.

## Tier contract

| Tier | Idle milestone | Responsibility | Current implementation |
|---|---:|---|---|
| Tier 0 | 0 ms | Revision tracking, dirty-range aggregation, timer reset, and native editor responsiveness. | `EditorAnalysisController::documentChanged` increments revision and starts both single-shot timers without copying full text or running language analysis. |
| Tier 1 | 150 ms | Fast lexical presentation using the existing block lexer/highlighter. | `TSyntaxHighlighter::runFastPass` rehighlights the dirty block range and one dependent block for legacy multiline state. |
| Tier 2 | 300 ms | Full language-core analysis and semantic presentation. | A worker receives an owned `QString` snapshot, runs `TaifLexer`, `TaifParser`, `SymbolTableBuilder`, and `SemanticPresentationAdapter`, then emits an immutable snapshot. |

> **Revision invariant:** Every edit increments the controller revision. A Tier 2 result is applied only when its revision equals the editor’s current revision. Late results are discarded and never mutate formats, completion, or future navigation state.

The timings are debounce milestones after the most recent edit, not a claim that full parsing must complete in exactly 300 milliseconds. Tier 0 has no whole-document analysis; Tier 2 is logically cancellable by revision and can safely finish in the background after a newer edit because its stale result is ignored.

## Implemented components

| Component | Role |
|---|---|
| `source/language/presentation/LanguageAnalysis.h` | Shared request, dirty-range, immutable snapshot, metrics, and presentation-span contracts. |
| `source/language/presentation/SemanticPresentationAdapter.*` | Pure Qt Core mapping from lexer/parser/semantic data to source-ranged presentation spans. |
| `source/texteditor/analysis/EditorAnalysisController.*` | Per-editor revision counter, single-shot 150/300 ms timers, worker lifetime, logical cancellation, and revision gate. |
| `source/texteditor/analysis/SemanticCompletionProvider.*` | Scope-aware suggestions from the current immutable `SemanticModel`. |
| `TSyntaxHighlighter` | Sole rendering highlighter. It keeps the legacy `TLexer` base layer and overlays only current-revision semantic spans. |
| `TEditor` | Owns the controller, captures document text only on the GUI thread at Tier 2 dispatch, triggers Tier 1, applies Tier 2 snapshots, and uses semantic completions when current. |

## Threading and lifecycle rules

The GUI thread owns `QTextDocument`, `TEditor`, `TSyntaxHighlighter`, timers, and all formatting calls. The worker owns no widget/document pointer and receives only an `AnalysisRequest` value containing revision, source string, and dirty metadata. It creates no `QTextCharFormat` and cannot call editor APIs.

`EditorAnalysisController` is a child of each `TEditor`. Its destructor stops timers, invalidates the shared latest revision, quits the worker thread, waits for it, and releases the current snapshot. Signals use Qt queued delivery; result acceptance verifies revision before assigning the semantic snapshot to the highlighter.

## Presentation policy

The existing highlighter remains the single `QSyntaxHighlighter` for the document. Its block order is legacy lexical formats first, semantic span overlay second, and diagnostic underline overlay last. Semantic classes map to existing theme categories where possible: functions/classes/builtins receive their corresponding theme format; parameters/locals use identifier color; unresolved names, duplicates, and errors receive wave underlines. `SEM999` is not rendered as hundreds of error underlines.

## Completion policy

Static snippets, keywords, and builtins remain synchronous fallback suggestions. When the latest snapshot revision matches the editor revision, `SemanticCompletionProvider` adds symbols visible at the cursor and skips the document-wide regex `DynamicWordStrategy`. This prevents the semantic system from re-parsing source in the completion path and avoids scope-blind duplicate suggestions.

## Validation record

| Check | Result |
|---|---|
| `TaifAnalysisTests` | **6 passed, 0 failed** on Windows Qt 6.11.1 / MSVC 2022. |
| Test coverage | Tier-zero immediate revision change, timer coalescing, latest-snapshot revision gate, and semantic span classification. |
| Full application build | Completed successfully; `Taif.exe` and all new analysis/presentation objects were produced. |
| Threading boundary | Worker test target links only language-core/controller code; no editor widget or `QTextDocument` is passed to the worker. |

## Remaining milestones

| Priority | Work | Exit criterion |
|---:|---|---|
| P0 | Add visual/editor integration tests for multi-block overlays, RTL source ranges, and highlighter automatic-rehighlight behavior. | Semantic spans remain correct during rapid edits, theme switches, and multiline strings. |
| P1 | Add semantic diagnostics UI, definition/reference commands, outline, and semantic folding behind feature flags. | All features consume only current-revision snapshots. |
| P1 | Add source-model completion context and remove dynamic-word fallback where semantic model is current. | Completion ranking respects shadowing and scope in actual popup interaction tests. |
| P2 | Add typed lexer checkpoints, parser subtree reuse, and semantic scope reuse. | Incremental results are equivalent to fresh results and profiling shows less work for local edits. |
