# Taif Parser and AST Plan

**Status:** Baseline parser, AST, recovery, symbol-table handoff, and safe reparse API implemented. Localized incremental relexing and green-node reuse remain planned.  
**Scope:** C++17/Qt Core parser architecture that consumes `LexResult` and produces immutable parse snapshots.  
**Validation state:** The dedicated Qt Test target is present; run it together with the full application build before merging.

## Goal

The Taif parser consumes the language-core lexer stream and always returns a non-null lossless syntax snapshot, semantic AST, and finite diagnostics. It is intentionally isolated from the highlighter and does not assign semantic name bindings. The following symbol-table tier consumes the AST through `SymbolTableInput` and owns scope construction, declarations, and name resolution.

> **Invariant:** Valid or malformed input must reach `EndOfFile` without an unbounded parser loop, crash, or loss of later declarations.

## Implemented baseline

| Area | Implementation | Primary files |
|---|---|---|
| Lexer contract | Parser consumes `LexResult` and preserves lexer diagnostics. | `source/language/lexer/TaifLexer.h` |
| Concrete syntax | Immutable-after-construction token snapshot with syntax nodes, error nodes, and missing-token nodes. | `source/language/parser/TaifParser.h/.cpp` |
| Semantic AST | Source-ranged `AstModule` with stable node IDs, declarations, expressions, suites, formatted-string parts, and errors. | `source/language/parser/TaifParser.h/.cpp` |
| Parsing strategy | Recursive descent for statements/layout; Pratt parsing for expressions and postfix chains. | `source/language/parser/TaifParser.cpp` |
| Recovery | Context-specific boundaries, missing tokens, error nodes, progress guard, and maximum diagnostic count. | `source/language/parser/TaifParser.cpp` |
| Symbol-table handoff | `SymbolTableInput` exposes only AST, diagnostics, and document revision. | `source/language/parser/TaifParser.h` |
| Tests | Lexer-to-parser, declarations, expressions, f-strings, malformed source, reparse fallback, and corpus regression. | `tests/parser/` |

## Parse pipeline

```text
QString source
     |
     v
TaifLexer::lex()
     |
     v
LexResult { tokens, lexicalDiagnostics }
     |
     v
TaifParser::parse()
     |
     +-------------------------+
     v                         v
SyntaxTree                 AstModule
(lossless tokens/CST)      (semantic nodes)
     |                         |
     |                         v
     |                    SymbolTableInput
     |                         |
     v                         v
editor tooling            scopes / symbols / reference binding
```

## Parser contract

```cpp
[[nodiscard]] ParseResult TaifParser::parse(
    const QString& source,
    const LexResult& lexicalResult,
    quint64 documentRevision = 0) const;

[[nodiscard]] ParseResult TaifParser::parse(
    const QString& source,
    quint64 documentRevision = 0) const;
```

The first overload is canonical. The second calls `TaifLexer` once and delegates to the canonical overload. `ParseResult` contains the full immutable syntax snapshot, AST, lexer diagnostics, parser diagnostics, and source revision.

## Grammar coverage

The baseline recognizes modules, newline/semicolon-separated statements, indentation suites, functions, classes, imports, from-imports, if/else-if/else, loops, try/except/else/finally, return/delete/break/continue, assignments, expressions, calls, members, index/slice operations, collections, comprehensions, lambda expressions, and structured formatted strings.

The precise grammar and recovery matrix are maintained in `documents/TaifParserGrammar.md`. Every grammar change must update that document first, add a focused test, and add a regression expectation where the behavior occurs in `Status.alif`.

## Error recovery policy

| Situation | Action | Safe boundary |
|---|---|---|
| Required token absent | Add zero-width `MissingToken` and `PAR001`. | Context-specific; do not consume a safe next token. |
| Invalid statement | Retain `ErrorNode`, then synchronize. | Newline, Arabic semicolon, dedent, EOF. |
| Invalid expression | Retain `ErrorExpression`, consume one non-boundary token. | Comma, colon, closer, newline, semicolon, dedent, EOF. |
| Unclosed delimiter | Add missing closing token. | Matching closer, layout boundary, EOF. |
| No parser progress | Emit `PAR099` and consume one token. | Immediate. |

## Current incrementality status

`TaifParser::reparse()` is available now as a correct, revision-aware full-parse fallback. It returns the same result as a fresh parse and identifies the fallback explicitly. This is intentional: the lexer does not yet expose typed checkpoints or partial relexing, so localized reparse would be unsound.

The next incremental milestone adds a lexer checkpoint/token-diff API, selects the smallest reparsable syntax ancestor, expands to a lexical/layout stability boundary, reparses only that region, and reuses unchanged immutable green subtrees. Incremental and fresh parses must be tested for equivalent CST, AST, and diagnostics before editor scheduling is enabled.

## Follow-on tasks

| Priority | Task | Exit criterion |
|---:|---|---|
| P0 | Confirm Taif-specific operator meanings and binding powers. | Parser grammar table and precedence tests are language-owner approved. |
| P0 | Confirm all legacy keywords not represented in `Status.alif`. | Lexer and parser token/grammar fixtures exist for every keyword. |
| P1 | Define the f-string format-spec mini-language. | `FStringFormat` lowers from raw text to specified semantic format nodes. |
| P1 | Add a real green/red immutable tree representation. | Syntax children are structurally shared across incremental parses. |
| P1 | Add lexer checkpoints and token-diff relexing. | Reparse window is localized and equivalent to a fresh parse. |
| P2 | Symbol-table baseline | Implemented in `source/language/semantic/`; functions, classes, parameters, imports, locals, and lexical references resolve from AST without token access. |
| P2 | Integrate document revision scheduling with the editor/highlighter. | No stale parse or semantic snapshot can be applied to a newer revision. |

## Validation requirements

The parser test target must pass focused AST/CST/recovery tests and the complete `Status.alif` corpus regression. Test assertions must check tree non-nullness, exact source ranges, expected node kinds, bounded diagnostics, later-declaration survival after an error, and end-of-file consumption. Performance work begins only after this correctness suite is stable and records tokens consumed, node count, diagnostic count, parse duration, and reuse ratio.
