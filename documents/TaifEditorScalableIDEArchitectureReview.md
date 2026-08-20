# TaifEditor: Scalable IDE Architecture Review and Roadmap

**Review scope:** Read-only review of the TaifEditor Qt 6/C++17 codebase, its language pipeline, editor shell, analysis system, semantic tools, diagnostics, build topology, tests, and living architecture documents.  
**Assessment date:** August 2026.  
**Primary conclusion:** **TaifEditor already has a strong single-document, Arabic-first language-service editor foundation.** To become a scalable IDE, it should next invest in stable language rules, document/workspace identity, controlled UI decomposition, and measurable incremental analysis—not add isolated UI features on top of the current single-document architecture.

## Executive assessment

The project has passed the difficult first architectural milestone: it does not treat highlighting as language understanding. Its lexer, parser, semantic model, editor feature adapters, revisioned snapshots, and range-safe UI features are separated well enough to support completion, hover, go-to-definition, diagnostics, and class-member intelligence without text-based guessing. The 0/150/300 ms analysis design, immutable worker output, UTF-16 range contract, parser recovery, and conservative handling of unknown external members are all decisions worth preserving.[1] [2] [3]

The main risk is now **scaling composition**, rather than basic language correctness. The language core intentionally performs full re-analysis. `TEditor` and `Taif` have accumulated many feature and lifecycle responsibilities. Workspace, project, module-resolution, cross-file indexing, and automated delivery foundations do not yet exist. Adding cross-file navigation, rename, signature help, quick fixes, or project diagnostics before those foundations would create fragile special cases and risk incorrect editor behavior.

> **Recommended strategy:** Evolve the existing architecture through explicit service boundaries. Preserve the current language-core and revision contracts; introduce document/workspace identity and shared query services; then add an incremental scheduler and project index. Avoid a rewrite.

| Readiness area | Current state | Assessment | Next architectural move |
|---|---|---|---|
| Single-document language intelligence | Lexer, parser, scopes, completion, hover, definition, diagnostics are implemented. | Strong foundation. | Consolidate shared semantic queries and add type/member facts. |
| Editor correctness | UTF-16 ranges, snapshot revision gates, recovery, stale-result rejection. | Strong; must remain invariant. | Add UI/highlighter regressions around edits and overlays. |
| Performance scalability | Full-document Tier 2 analysis, per-editor worker, no reuse/checkpoints. | Correctness-first but limited. | Establish benchmarks, then introduce scheduler/checkpoints/green-tree reuse. |
| UI architecture | Rich RTL surfaces and one dockable Problems panel; large editor/shell classes. | Functional but increasingly coupled. | Introduce sessions, controllers, command/dock/decorations services. |
| Workspace/IDE scale | Tabs and file browser; no workspace/module/project index. | Not yet an IDE substrate. | Build document registry, module resolver, project index, and task model. |
| Delivery maturity | Dedicated qmake tests exist; no root CI, benchmark/fuzz target, or project build abstraction. | Local validation is good; delivery automation is incomplete. | Add root orchestration/CI first; migrate builds only with compatibility proof. |

## Architecture strengths to preserve

### 1. UTF-16 source-range discipline

Lexer positions, parser/AST ranges, semantic declarations/references, diagnostics, highlighting, hover, definition, and editor selection all use half-open UTF-16 offsets. This exactly matches `QString` and `QTextDocument`, which is particularly important for Arabic text and mixed-direction code. This contract must remain the single source-coordinate system for future edits, references, rename previews, and cross-file locations.[2] [3]

### 2. Pure language core and immutable snapshots

The language core does not depend on `QTextDocument`, widgets, or syntax-theme code. The editor sends an owned source snapshot to a worker; the worker produces an immutable analysis snapshot; GUI consumers apply it only when the revision matches. This separation is the correct basis for background analysis, testing, future workspace indexing, and eventually an LSP adapter if one is desired.[1] [2]

### 3. Recovery-first parsing and bounded diagnostics

The parser is designed to reach EOF even with malformed input and retain later declarations. Semantic diagnostics are bounded with a suppression summary. That is the correct editor-first policy: one typo must not cascade into an unusable UI or uncontrolled work.[2] [4]

### 4. Semantic features consume semantic data, not regex guesses

Completion, hover, definition, and diagnostics are grounded in scopes, symbols, references, and exact ranges. Go-to-definition correctly refuses unresolved, external, malformed, and stale targets rather than fabricating destinations. This conservative behavior is a professional IDE characteristic and should be retained when cross-file functionality is added.[3] [5]

### 5. RTL-first surfaces are already part of the design

Completion, hover, semantic link affordances, and Problems use Arabic labels and RTL-aware placement. The project should treat RTL, accessibility, and bidirectional editing as architecture requirements, not final polishing tasks.

## Principal findings and recommendations

| ID | What was found | Why it matters | Recommended action |
|---|---|---|---|
| F-01 | Taif language policy is partially encoded in scanner/parser/semantic tables and corpus assumptions. | Module resolution, type/member behavior, refactoring, diagnostics, and tooling cannot be correct without stable rules. | Publish a versioned language specification for identifiers, aliases, indentation, operators/precedence, literals, f-strings, imports/exports, type annotations, inheritance, and visibility. |
| F-02 | The parser and semantic layer expose incremental seams but intentionally use full fallback/rebuild behavior. | Incrementality without lexer state and syntax identity would be unsound. | Add typed lexer checkpoints, token-diff invalidation, immutable green syntax nodes, and scope reuse only after fresh/incremental equivalence tests exist. |
| F-03 | `TEditor` owns many unrelated feature lifecycles; `Taif` combines shell, tabs, files, commands, tools, settings, and styling. | Event-order coupling and implementation cost grow rapidly as IDE features are added. | Introduce a `DocumentSession`, narrow feature controllers, command registry, dock manager, theme manager, and decoration manager. |
| F-04 | Current document identity is split between an editor field and dynamic QObject path properties. | Cross-file navigation, session restore, project indexing, and task diagnostics require one canonical identity. | Create `DocumentId` plus canonical URI/path, revision, encoding, modified state, snapshots, diagnostics, and navigation history in a session object. |
| F-05 | Each editor owns a worker thread; workers perform full-stage logical cancellation only. | Many tabs create unnecessary threads; stale work and close latency rise with large files. | Introduce a bounded shared analysis scheduler after document sessions exist. Give active documents priority and use cooperative cancellation checkpoints. |
| F-06 | Tier 1 is based on legacy per-block lexical state; semantic overlay can remain visible until new Tier 2 output arrives. | Fast presentation can temporarily diverge after multiline edits, and old semantic decorations can describe new text. | Add convergence and rapid-edit tests; clear/gate stale semantic overlays at Tier 0; replace legacy state with typed checkpoint caching in the incremental milestone. |
| F-07 | Semantic query helpers are partly duplicated across hover/definition and several query paths scan vectors on demand. | Rename, references, code actions, signature help, and high-frequency Ctrl-hover will duplicate logic and scale poorly. | Create a shared `SemanticQueryService` plus immutable interval/query indexes in snapshots. |
| F-08 | No workspace/model index exists; imports are local bindings and external symbols remain intentionally unresolved. | Cross-file definition, references, rename, global diagnostics, and module completion cannot be safely added. | Build `WorkspaceManager`, `DocumentRegistry`, `ModuleResolver`, import graph, export summaries, and `ProjectIndex` before cross-file UI features. |
| F-09 | Tests are well separated by layer but there is no root CI, fuzz/benchmark target, or UI interaction suite. | Correctness and responsiveness regressions can escape local manual builds. | Add root test orchestration, CI, corpus benchmarks, Unicode/property fuzzing, UI smoke tests, and latency budgets. |
| F-10 | Styling, commands, tool hosting, and temporary visual decorations are distributed. | Accessibility, RTL consistency, future docks, keyboard parity, and overlay interactions will drift. | Centralize design tokens, commands, dock policy, and named decoration layers. |

## Target scalable IDE architecture

The target is an **evolutionary architecture**, not a replacement of the current editor. The current language core becomes the document-language service; the editor becomes a view bound to a document session; workspace services compose document facts into project facts.

```text
Application / Workspace Window
  ├── CommandRegistry + Action bindings
  ├── DockManager + WorkspaceSession persistence
  ├── DocumentManager
  │     └── DocumentSession { DocumentId, URI, revision, text, diagnostics, history }
  ├── FileExplorerModel (workspace-root projection)
  ├── TaskRunner (run/build/test output)
  └── ThemeManager + RTL/accessibility policy

DocumentSession
  ├── TEditor view surface
  ├── Editor feature controllers
  │     ├── CompletionController
  │     ├── HoverController
  │     ├── NavigationController
  │     ├── DiagnosticController
  │     ├── Folding/Minimap/Autosave controllers
  │     └── DecorationManager
  └── AnalysisScheduler task submission

Language services
  ├── IncrementalLexer -> token store/checkpoints
  ├── IncrementalParser -> green CST / source views / semantic AST
  ├── SemanticDocumentModel -> scopes, symbols, references, diagnostics, indexes
  └── SemanticQueryService -> completion, hover, definition, references, rename validation

Workspace services
  ├── ModuleResolver
  ├── ImportGraph
  ├── Export summaries
  ├── ProjectIndex
  └── ProjectQueryService -> cross-file navigation, references, global diagnostics
```

The dependency direction must remain strict: **widgets depend on services; services depend on immutable model contracts; language core does not depend on widgets or workspace UI**. Project indexes may consume document snapshots, but individual document analysis must remain correct without an open workspace.

## Delivery roadmap

### Horizon A — Stabilize the foundation

This is the highest-value engineering work because it makes later capabilities safe rather than merely visible.

| Milestone | What | Why now | Acceptance criteria |
|---|---|---|---|
| A1. Language rules and test matrix | Formalize approved Taif language semantics and grammar decisions. | Prevents later IDE behavior from depending on accidental implementation choices. | Versioned specification; each decision has positive/negative lexer/parser/semantic tests. |
| A2. Document session | Add one canonical `DocumentId`/URI/session object; remove split file-path state. | Required by every workspace, task, and cross-file feature. | Opening/saving/tabs/diagnostics/navigation use one session identity; session tests pass. |
| A3. UI safety refactor | Add tests, then extract diagnostics/navigation and a command registry. | Reduces current event coupling before new features arrive. | Existing interactions unchanged; no duplicate tab signal connections; commands testable by ID/context. |
| A4. Analysis correctness/telemetry | Clear stale overlays; add highlighter convergence tests; capture latency metrics. | Preserves trust while building future performance work. | Rapid-edit tests prove no stale decorations; telemetry records timing/queue/stale metrics. |
| A5. Root validation/CI | Add one developer command and CI for all Qt tests/full build. | Makes architectural refactoring safe and repeatable. | Fresh checkout runs lexer/parser/semantic/analysis/full-build validation automatically. |

### Horizon B — Establish a workspace substrate

| Milestone | What | Why after Horizon A | Acceptance criteria |
|---|---|---|---|
| B1. Workspace roots and persistence | Workspace session, recent roots, document restoration, dock state schema. | Requires document identity and command/dock boundaries. | Reopen restores files, active document, cursor/scroll, and dock layout robustly. |
| B2. Module resolver/import graph | Canonical import spelling-to-document policy and dependency graph. | Requires approved import semantics. | Handles relative/standard/project imports, cycles, missing modules, and invalidation deterministically. |
| B3. Project index/export summaries | Publish per-file symbols/diagnostics with document IDs and revisions. | Cross-file features need verified facts, not source scans. | Closed/open document updates converge; export change invalidates dependents correctly. |
| B4. Cross-file definition and global Problems | Extend existing precise local features through project query results. | Builds directly on B2/B3. | Destination URI/range is verified; global diagnostics group by document and navigate safely. |

### Horizon C — Expand trusted language intelligence

| Milestone | What | Prerequisite | Acceptance criteria |
|---|---|---|---|
| C1. Outline and local references | UI over current `documentSymbols()` and reverse-reference data. | A2 controller/command/decorations. | Select symbol, show references, navigate via exact ranges. |
| C2. Cross-file references and rename preview | Project symbol/reference index with conflict analysis. | B3 and formal identifier rules. | Preview is complete/explicitly partial; atomic workspace edit has undo grouping and collision checks. |
| C3. Type/member facts and signature help | Declared class shapes, receiver facts, inheritance/static policy, callable metadata. | Language type/member decisions. | Nested member chains and calls resolve conservatively; unknown stays external. |
| C4. Formatting, linting, and code actions | Lossless CST edit engine and structured diagnostics/fix metadata. | A1, parser identity, diagnostic taxonomy. | Non-overlapping edits are previewable/applicable and retain trivia/RTL source correctness. |

### Horizon D — Production maturity and extensibility

The project should treat this horizon as deliberate product work, not background cleanup. Add task/build configurations, debug integration policy, performance budgets, packaging/update strategy, accessibility audits, and a documented decision on LSP or plugins. An LSP adapter can be valuable after language/workspace/query APIs stabilize; it should not define their first version.

## Qt 6 / C++17 engineering guidance

### Object ownership and threading

Use QObject parent ownership for UI/controller trees, but do not make service lifetime implicit. `DocumentSession`, `WorkspaceManager`, and `AnalysisScheduler` should have explicit ownership in the application/workspace layer. Workers must remain free of widget and `QTextDocument` pointers. Use immutable value/smart-pointer results across queued signals; do not expose mutable model internals across threads.

Replace per-editor threads with a bounded scheduler only after the session layer exists. Favor `QThreadPool`/task objects or a dedicated scheduler thread strategy with bounded concurrency, cancellation tokens, and active-document priority. Keep full analysis as a correctness oracle while incremental work is experimental.

### Model/view and UI services

Continue using `QAbstractItemModel` for Problems and use the same approach for outline, references, files, tasks, and symbols. UI models should expose immutable value rows with IDs/ranges, while controllers perform navigation/query actions. Avoid placing semantic lookup or file system traversal inside view delegates/widgets.

### Command and decoration composition

Every command should have a stable ID, Arabic display text, shortcut, enablement predicate, execution handler, and context. Every transient visual effect should have a named layer/priority: current line, find match, semantic link, diagnostic selection, references, rename preview, breakpoint, and execution line. This avoids accidental `ExtraSelection` overwrites as features grow.

### Build evolution

Keep qmake stable for current users. First create root validation scripts and CI that prove the existing qmake targets. Only then create a parallel CMake graph and require equivalent source registration, generated Qt/MOC behavior, test results, and Windows build output before treating CMake as the primary build. A build migration without that gate is risk, not modernization.

## Arabic/RTL and accessibility requirements

Arabic-first behavior should remain a first-class acceptance criterion for every feature. The IDE needs tests for Arabic identifiers, mixed Arabic/ASCII punctuation, UTF-16 ranges, bidi caret movement, selections, copy/paste, tooltip/dock placement, keyboard navigation, and screen-reader names. All commands need keyboard-only access. Themes need contrast rules for normal, selected, warning, error, disabled, and linked states.

| Requirement | Practical policy |
|---|---|
| Text direction | Apply RTL to Arabic UI prose while preserving code-token and numeric source semantics. Test mixed bidi content explicitly. |
| Navigation | F12, Ctrl-click, context actions, diagnostic activation, outline/reference rows, and dock controls must have keyboard equivalents. |
| Accessibility | Assign accessible names/descriptions; verify focus order, popup focus transfer, high contrast, and non-colour severity labels. |
| Localization | Keep stable diagnostic codes separate from localized message catalogs; do not parse display text for behavior. |
| Visual consistency | Derive autocomplete, hover, Problems, outline, and references from shared semantic colours/design tokens. |

## Decisions needed from the Taif language/product owner

The following decisions should be made before engineering dependent capabilities:

| Decision | Blocks |
|---|---|
| Project root/configuration and import/package resolution | Workspace, module graph, cross-file definition, project diagnostics. |
| Identifier normalization and accepted Unicode policy | Rename, duplicate detection, formatter, cross-platform file/module matching. |
| Operators and precedence | Parser correctness, type analysis, diagnostics, formatter. |
| Annotations/types, inheritance, static/class/instance behavior, overloads | Member completion, signature help, rename safety, type diagnostics. |
| F-string format-spec and escape grammar | Parser/formatter/highlighting correctness. |
| Export/visibility and standard library module model | Public symbol index, external definitions, module completion. |
| Supported workspace sizes/platforms and run/debug expectations | Scheduler budgets, project model, CI matrix, packaging, task runner. |

## Recommended next three engineering milestones

1. **Language and document identity foundation.** Produce the approved Taif semantic/import specification, add `DocumentSession`/`DocumentId`, remove split path state, and add session/tab lifecycle tests.
2. **Analysis trust and delivery automation.** Clear/gate stale semantic overlays on edit, add highlighter convergence/rapid-edit tests, collect latency metrics, and create root test/build CI.
3. **Workspace index pilot.** Implement workspace roots, canonical module resolution, per-file export summaries, and a revisioned project index; use it first for cross-file definition, not rename.

These three milestones create the prerequisites for the next visible IDE gains—outline, references, cross-file navigation, global Problems, rename preview, and type-aware signatures—while protecting the correctness and polished Arabic-first behavior already achieved.

## Evidence references

[1]: [Three-Tier Analysis Pipeline](TaifEditorAnalysisPipelinePlan.md)
[2]: [Parser Architecture Plan](TaifParserPlan.md)
[3]: [Symbol Table Architecture Plan](TaifSymbolTablePlan.md)
[4]: [Semantic Rules](TaifSemanticRules.md)
[5]: [Go-to-Definition Contract](TaifGoToDefinition.md)
[6]: [Diagnostics and Problems Panel Contract](TaifDiagnosticsAndProblemsPanel.md)
[7]: [Lexer Architecture Plan](TaifLexerPlan.md)
