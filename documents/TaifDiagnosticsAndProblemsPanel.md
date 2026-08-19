# Diagnostics and Problems Panel

**Status:** Implemented and validated  
**Applies to:** Qt 6 / C++17 TaifEditor language analysis, semantic rendering, and main-window UI

## Purpose

TaifEditor now exposes one revision-safe diagnostics system across the editor and the main window. It renders **red jagged wave underlines for errors**, **yellow jagged wave underlines for warnings**, and presents current-document problems in the dockable RTL **`المشكلات`** panel at the bottom of the editor window.

> **Diagnostic invariant:** The highlighter and panel both consume the same immutable `LanguageAnalysisSnapshot::diagnostics` vector. A document edit clears the editor’s diagnostics immediately, so an old worker result cannot decorate or populate newer source text.

## Architecture

The diagnostics flow is built on the existing Tier 2 worker pipeline and does not parse source again in GUI code.

| Component | Responsibility |
|---|---|
| `DiagnosticPresentationAdapter` | Pure lexer/parser/semantic diagnostic normalization, sorting, and deduplication. |
| `EditorDiagnostic` | Immutable code, message, UTF-16 range, severity, and source-stage origin. |
| `LanguageAnalysisSnapshot` | Carries normalized diagnostics and severity-aware presentation spans for one revision. |
| `SemanticPresentationAdapter` | Emits diagnostic spans after normal semantic spans, preserving diagnostic severity. |
| `TSyntaxHighlighter` | Applies wave-underlines without disturbing normal token or semantic colours. |
| `TEditor` | Publishes only current-revision diagnostics, clears them on edits, and selects exact ranges on panel activation. |
| `DiagnosticsPanel` | Dark RTL table model/view with counters, filters, empty state, and row activation. |
| `Taif` | Owns the bottom `QDockWidget`, keeps it synchronized with the active tab, and routes activation to the current editor. |

## Diagnostic normalization

`DiagnosticPresentationAdapter` reads all language-core diagnostics after lexer, parser, and semantic analysis finish. It preserves parser and semantic severities, treats current lexer diagnostics as errors, and ignores the parser’s forwarded lexical copy because the lexer result is the canonical source.

| Origin | Severity policy | Notes |
|---|---|---|
| Lexer | Error | Current lexer records do not carry a separate severity. |
| Parser | Error or Warning | Preserves `ParseDiagnosticSeverity`. |
| Semantic | Error, Warning, or Information | Preserves the language-core severity exactly. |

Entries are deduplicated by origin, code, message, range, and severity, then sorted by source offset, severity, and code. Information entries remain in the immutable snapshot for future use but are not displayed by default in the current Problems table and do not receive a wave underline.

## Editor underline policy

The existing semantic overlay highlighter remains the only renderer. Diagnostic spans are applied after lexical and semantic classification spans, so the wave decoration remains visible while token colour remains semantically correct.

| Severity | Underline | Colour |
|---|---|---|
| Error | `QTextCharFormat::WaveUnderline` | `#f06464` |
| Warning | `QTextCharFormat::WaveUnderline` | `#ffb446` |
| Information | None by default | Not applicable |

Empty or invalid ranges never create a highlighter span. This prevents formatting errors at document boundaries or suppression-summary locations.

## Problems dock

The bottom dock uses a real `QDockWidget`, so Qt’s standard docking, floating, hiding, resizing, and restoration behavior is retained. It is initially placed in `Qt::BottomDockWidgetArea` and styled with the editor’s dark-blue visual system.

| Panel region | Behavior |
|---|---|
| Header | RTL title `المشكلات`, total count, and compact severity filter buttons. |
| List | Severity, description, source location, and stable diagnostic code. |
| Error row | Red glyph and `خطأ` label. |
| Warning row | Yellow glyph and `تحذير` label. |
| Empty state | `لا توجد أخطاء أو تحذيرات في المستند الحالي`. |
| Activation | Double-click or Enter selects the exact UTF-16 diagnostic range and scrolls it into view. |

The dock always reflects the active editor tab. On tab switch it reads that editor’s current immutable diagnostics; on edits it clears immediately until the next accepted Tier 2 snapshot arrives.

## Range and revision safety

Diagnostic navigation checks the source range before moving the cursor. Invalid, empty, or out-of-document ranges are ignored. Panel activation never changes source text. Completion, hover, and Ctrl-hover definition-link surfaces are dismissed before selecting a diagnostic range so the destination remains clear.

## Validation

| Validation target | Result |
|---|---|
| Lexer suite | Passed: 10 tests. |
| Parser suite | Passed: 12 tests. |
| Semantic suite | Passed: 12 tests. |
| Analysis, hover, definition, and diagnostics suite | Passed: 17 tests. |
| Full TaifEditor Qt 6.11.1 / MSVC 2022 build | Passed. |

The analysis regressions cover lexer/parser normalization, lexical-forwarding deduplication, severity-preserving diagnostic spans, panel-model filtering and counters, and existing semantic diagnostic presentation.

## Extension path

The current panel is intentionally local to the active document. A project/module index can later add file identity to `EditorDiagnostic` and enable cross-file activation. Future features may include quick fixes, suppression actions, build/runtime diagnostics, project-wide grouping, `Information` filters, diagnostic code documentation, and accessibility announcements. They must preserve the current immutable-snapshot and UTF-16 range contracts.
