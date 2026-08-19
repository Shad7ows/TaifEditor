# Taif Symbol Table and Semantic Model Plan

**Status:** Baseline scope builder, declaration index, lexical reference resolver, diagnostics, editor-query API, and integration test target implemented. Incremental scope reuse and editor widget integration remain planned.  
**Validation state:** `TaifSemanticTests` passed 10/10 on the Windows Qt 6.11.1 / MSVC 2022 toolchain after diagnostic-cascade recovery. Re-run lexer/parser tests and the full application build before merging the complete language stack.

## Goal

The semantic layer converts a parser-produced `AstModule` into an immutable, revisioned `SemanticModel`. The model contains a lexical scope tree, declarations, name references, resolution results, semantic diagnostics, and source-ranged queries for editor features. It intentionally performs no source scanning and has no widget dependency.

> **Invariant:** a non-null parser AST produces a non-null finite semantic model, even when source has parser errors or unresolved names.

## Implemented architecture

| Area | Baseline implementation | Primary files |
|---|---|---|
| Parser contract hardening | `AstChildRole` metadata and assignment target-count field prevent semantic analysis from guessing child positions. | `source/language/parser/TaifParser.h/.cpp` |
| Semantic data model | IDs and tables for scopes, symbols, references, diagnostics, and revision. | `source/language/semantic/SymbolTable.h` |
| Declaration pass | Prelude/module/function/class/lambda/comprehension scope creation, two-pass declarations, parameters, imports, assignments, and loop targets. | `source/language/semantic/SymbolTable.cpp` |
| Resolution pass | Lexical parent-chain resolution, recursion, shadowing, calls, members, assignments, deletes, imports, and formatted/ordinary expression traversal. | `source/language/semantic/SymbolTable.cpp` |
| Editor query API | Visible symbols, source-offset reference lookup, reverse references, document symbols, and revision exposure. | `source/language/semantic/SymbolTable.h/.cpp` |
| Rules documentation | Scope rules, semantic roles, diagnostics, and editor contract. | `documents/TaifSemanticRules.md` |
| Tests | Parser-to-semantic integration, scopes, shadowing, imports, diagnostics, queries, malformed source, and corpus regression. | `tests/semantic/` |

## Semantic pipeline

```text
TaifParser
    |
    v
ParseResult { AstModule, parser diagnostics, revision }
    |
    v
SymbolTableBuilder::build(SymbolTableInput)
    |
    +--> Scope tree
    +--> Symbol table
    +--> Name references / bindings
    +--> Semantic diagnostics
    +--> Editor query indexes
    |
    v
shared_ptr<const SemanticModel>
```

The `previous` semantic-model parameter is accepted at the API boundary but is intentionally unused in the correctness-first baseline. It must not be used for subtree reuse until parser AST identity and typed incremental relexing exist.

## Scope baseline

Module, function, class, lambda, and comprehension scopes are separate. Ordinary suites, conditionals, loops, and try blocks share their containing lexical scope. Function/class symbols are indexed before body resolution. Parameter defaults resolve in the enclosing scope. Loop iterable resolves before target binding; comprehension iterable resolves outside the temporary comprehension scope.

The full approved table of scope/binding rules, AST role requirements, diagnostics, and editor data is in `TaifSemanticRules.md`.

## Remaining milestones

| Priority | Task | Exit criterion |
|---:|---|---|
| P0 | Run and repair complete language-stack validation. | `TaifSemanticTests` passed 10/10; `TaifParserTests`, lexer tests, and the application build must be rerun after the AST-role contract change. |
| P0 | Confirm Taif language semantics for special names, class/instance binding, scope declarations, and operator/runtime rules. | Rules document and semantic tests are language-owner approved. |
| P1 | Replace compact parser AST positions with complete typed payload/roles where not yet explicit, especially lambda parameters and exception binders. | Semantic layer has no `SEM005` gap for approved syntax. |
| P1 | Add a prelude configuration/project import index. | External import/type symbols resolve through a project snapshot rather than placeholders. |
| P1 | Add revision-aware semantic document controller and completion adapter. | Scope-aware completion supersedes the scope-blind document regex strategy without stale results. |
| P2 | Add type/member analysis. | Member references resolve to declared class/module members and power hover/type hints. |
| P2 | Add localized semantic reuse. | Incremental semantic model is equivalent to fresh output and reuses only parser-identity-safe subtrees. |

## Validation requirements

Semantic tests must assert scope ownership, declaration kinds/ranges, lexical shadowing and closure behavior, recursion, imports/prelude, duplicate/unresolved diagnostics, member external state, visible-symbol order, definition/reference index behavior, malformed-source survival, corpus boundedness, and revision correctness. The full `Status.alif` corpus is a regression input, not a complete semantic specification.
