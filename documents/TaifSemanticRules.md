# Taif Semantic Scope and Symbol Rules

**Status:** Initial semantic-model baseline. Update this document before changing scope, declaration, or reference behavior.  
**Input:** Parser `AstModule`, parser diagnostics, and document revision through `SymbolTableInput`.  
**Output:** Immutable `SemanticModel` containing scopes, symbols, references, semantic diagnostics, and editor-query indexes.

## Semantic boundary

The symbol table is a Qt Core-only analysis layer. It consumes the parser AST; it does not read `QString` source, lexer tokens, highlighter state, `QTextDocument`, or widgets. All positions are half-open UTF-16 ranges and therefore map directly to Qt editor selections.

> A semantic build always returns a model for a non-null AST. Invalid or incomplete syntax becomes partial symbols, unresolved references, or source-ranged diagnostics rather than an analysis failure.

## AST role contract

The parser annotates each AST child with an `AstChildRole` and records `assignmentTargetCount` on assignment statements. The semantic layer must use these roles rather than inferring declaration boundaries from child index alone.

| AST node | Required roles | Semantic interpretation |
|---|---|---|
| `FunctionDeclaration` | `DeclarationName`, `ParameterList`, `Body` | Declare a function in the parent scope; create its child scope. |
| `ClassDeclaration` | `DeclarationName`, zero or more `Base`, `Body` | Declare class in parent; resolve bases in parent; create class scope. |
| `Parameter` | `ParameterName`, optional `DefaultValue` | Bind parameter in function/lambda scope; resolve default in enclosing scope. |
| `AssignmentStatement` | One or more `Target`, one `Value`; target count metadata | Bind valid lexical targets, then resolve RHS. |
| `ForStatement` | `Target`, `Iterable`, `Body` | Resolve iterable first, bind target in containing scope, then analyze body. |
| `ComprehensionExpression` | `Element`, `Target`, `Iterable` | Resolve iterable in parent; bind target and resolve element in temporary comprehension scope. |
| `ImportStatement` | `ImportPath` | Bind final module segment as an external/import-module symbol. |
| `FromImportStatement` | `ImportPath`, one or more `ImportName` | Bind imported local names as external/import-member symbols. |
| `MemberExpression` | `MemberBase`, `MemberName` | Resolve base lexically; defer member to future type/member analysis. |
| `CallExpression` | `Callee`, zero or more `Argument` | Resolve callee as a call and arguments as reads. |

## Scope policy

| Scope kind | Creates scope | Parent | Declarations |
|---|---|---|---|
| Prelude | Once per semantic model | None | Builtins and configured globals. |
| Module | Once per document | Prelude | Top-level functions, classes, imports, assignments. |
| Function | Each function | Lexical enclosing scope | Parameters, assignments, nested functions/classes, loop variables. |
| Class | Each class | Lexical enclosing scope | Methods, nested classes, class assignments. |
| Lambda | Each lambda | Lexical enclosing scope | Parameters once parser emits explicit lambda roles. |
| Comprehension | Each comprehension | Lexical enclosing scope | Comprehension target only. |
| Ordinary suite | No | Containing scope | No separate scope in the current Python-like baseline. |
| `if` / `while` / `try` | No | Containing scope | No separate scope in the current baseline. |
| `for` | No | Containing scope | Target binds in containing scope. |

The language owner must confirm Taif-specific semantics for `هذا`, class methods, inheritance, global/nonlocal declarations, decorators, exception binders, destructuring, and lambda defaults. Until confirmed, the semantic model favors partial information and `SEM005` informational diagnostics over invented rules.

## Resolution rules

Resolution walks lexical parent scopes from nearest to farthest: local scope, enclosing function/class scopes, module, then prelude. A declaration lookup at the nearest scope is resolved if exactly one candidate exists, or ambiguous if duplicate declarations share the same spelling in that scope.

Functions and classes are declared before their bodies are analyzed. The complete declaration-index pass runs before reference resolution, allowing recursion and forward editor navigation within one scope. Runtime initialization ordering is intentionally outside this first semantic layer.

Members and import paths remain `External` references until type analysis or a project/module index provides candidates. They do not create false `SEM001` undefined-name diagnostics.

## Diagnostics

| Code | Meaning | Severity in baseline |
|---|---|---|
| `SEM001` | Unresolved lexical read, call, or base name. | Warning |
| `SEM002` | Duplicate declaration or ambiguous reference. | Warning |
| `SEM003` | Invalid binding target. | Warning |
| `SEM004` | Invalid AST semantic-role invariant. | Warning |
| `SEM005` | Unsupported but recoverable scope-affecting construct. | Information |
| `SEM006` | Stale editor semantic snapshot. | Warning at adapter boundary |
| `SEM999` | Detailed semantic diagnostic budget reached. | Information; one suppression summary only |

Diagnostics are deduplicated by code, message, and range. To prevent incomplete corpus code from flooding editor underlines, the model keeps at most 96 detailed semantic diagnostics and then emits one `SEM999` informational suppression summary, for a maximum of 97 semantic diagnostics. Parser diagnostics remain a separate stream and are not duplicated as semantic diagnostics.

## Editor queries

The immutable model supports `visibleSymbolsAt`, `referenceAt`, `referencesOf`, and `documentSymbols`. These data-only queries are the required inputs for a later editor adapter that provides scoped completion, go-to-definition, hover, find references, rename preparation, outline, breadcrumbs, folding, and semantic underlines.

The editor must publish a semantic model only if its `documentRevision` equals the active document revision. Static keywords, snippets, and builtins remain available as completion fallbacks while scope-aware completion is integrated.
