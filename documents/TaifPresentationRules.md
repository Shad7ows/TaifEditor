# Taif Editor Semantic Presentation Rules

**Status:** Baseline overlay rules for the three-tier editor pipeline.  
**Scope:** Mapping lexer/parser/semantic data to the existing `TSyntaxHighlighter` without creating a competing highlighter.

## Ownership

`TSyntaxHighlighter` is the only `QSyntaxHighlighter` attached to a document. Tier 1 provides its legacy `TLexer` token formats. Tier 2 provides immutable data-only `PresentationSpan` values; it never creates `QTextCharFormat` off-thread and never accesses a widget or `QTextDocument` off-thread.

## Rendering order

| Order | Layer | Purpose |
|---:|---|---|
| 1 | Legacy block lexer | Fast theme-based strings, comments, keywords, numbers, and operators. |
| 2 | Semantic overlay | Declaration/reference-aware functions, classes, parameters, locals, imports, and builtins. |
| 3 | Diagnostics | Wave underline for unresolved names, duplicate declarations, and syntax/semantic errors. |

The semantic overlay only applies when the snapshot revision equals the current editor revision. A stale snapshot is discarded by `EditorAnalysisController` before reaching the highlighter.

## Theme mapping

| Presentation class | Existing theme category | Additional decoration |
|---|---|---|
| Function declaration / reference | `Function` | None |
| Class declaration / reference | `ClassDef` | None |
| Builtin | `BuiltinFunc` | None |
| Parameter / local | `Identifier` | None |
| Import | `Identifier` | None |
| Unresolved name | `Identifier` | Red wave underline |
| Duplicate declaration | `Identifier` | Amber wave underline |
| Error | `Identifier` | Red wave underline |

`SEM999` is a model-level suppression summary. It intentionally creates no source underline because its purpose is to avoid visual diagnostic flooding while the document is incomplete.

## Range policy

All spans use half-open UTF-16 ranges from the language-core source model. During `highlightBlock`, the highlighter intersects each span with the current block and converts the overlap to a block-relative offset. This preserves Arabic source positions because `QString`, `QTextDocument`, lexer, parser, and symbol table share the same UTF-16 coordinate system.
