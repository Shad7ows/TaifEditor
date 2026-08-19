# Taif Parser Grammar and AST Contract

**Status:** Initial implementation baseline.  
**Scope:** Parser grammar, CST/AST behavior, recovery rules, and the symbol-table boundary.  
**Input:** `LexResult` from the language-core Taif lexer.  
**Output:** A lossless `SyntaxTree`, semantic `AstModule`, and parser diagnostics.

## Contract

The parser receives a complete lexer snapshot. It retains all lexer tokens, including comments and whitespace trivia, in the immutable concrete syntax tree. Grammar decisions consume only main-channel tokens. The parser never derives indentation from source text and never uses highlighter token types.

> Every parse returns a non-null syntax tree and AST. Missing syntax becomes a `MissingToken` node; skipped syntax becomes an `ErrorNode`; parsing continues to a contextual synchronization boundary.

## Module and statements

```text
Module              ::= { StatementSeparator | Statement } EndOfFile
StatementSeparator  ::= Newline | ArabicSemicolon
Statement           ::= CompoundStatement | SimpleStatement

CompoundStatement   ::= IfStatement | ForStatement | WhileStatement
                      | TryStatement | FunctionDeclaration | ClassDeclaration
Suite               ::= Colon Newline Indent { StatementSeparator | Statement } Dedent
                      | Colon SimpleStatement

SimpleStatement     ::= ImportStatement | FromImportStatement | DeleteStatement
                      | ReturnStatement | BreakStatement | ContinueStatement
                      | AssignmentOrExpressionStatement
```

The parser accepts both lexer-supported comma forms in comma-separated contexts. It preserves the raw punctuation spelling in the CST; the AST stores only semantic child ordering.

## Declaration and scope grammar

```text
FunctionDeclaration ::= KwFunction Identifier ParameterList Suite
ClassDeclaration    ::= KwClass Identifier [ LParen ExpressionList RParen ] Suite
ParameterList       ::= LParen [ Parameter { Comma Parameter } [ Comma ] ] RParen
Parameter           ::= [ Star | DoubleStar ] Identifier [ Equal Expression ]

ImportStatement     ::= KwImport DottedName
FromImportStatement ::= KwFrom DottedName KwImport Identifier { Comma Identifier }
DottedName          ::= { Dot } Identifier { Dot Identifier }
```

`FunctionDeclaration`, `ClassDeclaration`, parameters, imports, and name expressions carry exact UTF-16 source ranges and stable AST IDs. These nodes are the direct inputs to the future symbol table. Function and class declarations create scope-forming constructs; `NameExpression` nodes deliberately remain unresolved until semantic analysis.

## Control-flow grammar

```text
IfStatement         ::= KwIf Expression Suite
                      { KwElseIf Expression Suite }
                      [ KwElse Suite ]
ForStatement        ::= KwFor Expression KwIn Expression Suite
WhileStatement      ::= KwWhile Expression Suite
TryStatement        ::= KwTry Suite
                      { KwExcept [ Expression ] Suite }
                      [ KwElse Suite ] [ KwFinally Suite ]
ReturnStatement     ::= KwReturn [ Expression ]
DeleteStatement     ::= KwDelete Expression
```

## Expression grammar

The parser uses Pratt parsing. Prefix expressions select a null denotation; calls, members, and indexing are parsed as high-binding postfix expressions; binary operators consult one centralized binding-power table.

| Binding level | Operators | Associativity | Status |
|---:|---|---|---|
| 10 | `او` | Left | Implemented baseline |
| 20 | `و` | Left | Implemented baseline |
| 30 | comparisons, `في`, `ليس في` | Left | Implemented baseline |
| 40 | `+`, `-` | Left | Implemented baseline |
| 50 | `*`, `\`, `\*`, `\\`, `%` | Left | Implemented baseline |
| 60 | `^` | Right | Implemented baseline; confirm runtime semantics |
| 70 | calls, members, index/slice | Left/postfix | Implemented baseline |

```text
Expression          ::= PrefixExpression { InfixOrPostfixExpression }
PrefixExpression    ::= Identifier | Literal | UnaryExpression | Collection
                      | ParenthesizedOrTuple | Lambda | FormattedString
UnaryExpression     ::= ( Plus | Minus | KwNot ) Expression
Call                ::= Expression LParen [ ArgumentList ] RParen
Argument            ::= Expression | Identifier Equal Expression
                      | Star Expression | DoubleStar Expression
IndexOrSlice        ::= Expression LBracket [ Expression ] [ Colon [ Expression ] [ Colon [ Expression ] ] ] RBracket
Lambda              ::= KwLambda IdentifierList Colon Expression
```

The Taif-specific meanings of `^`, `\`, `\*`, and `\\` require language-owner confirmation. The parser’s binding-power table is isolated so this decision changes one table and its tests rather than the grammar structure.

## Collections and formatted strings

```text
List                ::= LBracket [ ExpressionList | Expression KwFor Expression KwIn Expression ] RBracket
Tuple               ::= LParen [ ExpressionList ] RParen
Map                 ::= LBrace Expression Colon Expression { Comma Expression Colon Expression } RBrace
Set                 ::= LBrace Expression { Comma Expression } RBrace

FormattedString     ::= FStringStart { FStringText | Interpolation } FStringEnd
Interpolation       ::= InterpolationStart Expression [ Colon FStringFormat ] InterpolationEnd
```

Formatted-string text, interpolation expressions, and raw format specifications become separate AST parts. The parser carries `FStringFormat` as raw semantic text until a future language specification defines its format mini-language.

## Error recovery

| Diagnostic | Situation | Recovery |
|---|---|---|
| `PAR001` | Required token is absent. | Insert a zero-width `MissingToken` node without consuming a safe next token. |
| `PAR002` | Statement or suite body is invalid/missing. | Create error structure and synchronize to newline, semicolon, dedent, or EOF. |
| `PAR003` | Prefix expression is missing or invalid. | Retain an error expression and consume one non-boundary token. |
| `PAR004` | Invalid token appears inside formatted-string structure. | Retain it under an error node and continue to interpolation/string end. |
| `PAR099` | Progress guard detects a no-consumption parser loop. | Emit a guard diagnostic and consume one token. |

Recovery synchronization tokens are contextual. Expressions stop at commas, colons, closers, line boundaries, dedents, and EOF. Statements stop at line boundaries, Arabic semicolons, dedents, and EOF. A recovery branch must either consume a main-channel token, insert a missing node and return, or reach EOF.

## Symbol-table handoff

The semantic tier receives `SymbolTableInput { AstModule, parserDiagnostics, documentRevision }`. It must not inspect lexer tokens or concrete syntax. Its first responsibilities are to create module/function/class scopes, register declarations, bind parameters/imports/locals, and resolve `NameExpression` nodes. The parser intentionally performs no name resolution and emits no undefined-name error.

## Incremental parsing status

The current parser builds correct immutable snapshots from a full lexer result. It is designed for later green-node reuse but does not yet expose edit-based reparsing. Incremental parsing will be added only after token-diff/relex support exists in the lexer and fresh-parse versus incremental-parse equivalence tests are in place.
