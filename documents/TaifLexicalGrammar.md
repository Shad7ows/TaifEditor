# Taif Lexical Grammar

**Status:** Working baseline derived from `Status.alif`; update this document before changing lexer behavior.  
**Scope:** Lexical terminals, source layout, literals, comments, and error recovery.  
**Canonical source coordinate system:** Half-open UTF-16 ranges, one-based lines and columns.

## Purpose

This document defines the initial parser-facing lexer behavior for Taif. It is based on the checked-in language examples in `Status.alif`, particularly assignments, collections, imports, indentation-delimited blocks, strings, formatted strings, and classes. It is deliberately a lexical grammar rather than a complete expression or statement grammar: the parser decides precedence, declaration structure, and runtime meaning.

> A conforming lexer consumes all finite input, emits explicit grammar terminals in source order, produces an `EndOfFile` token, and reports invalid lexical constructs with a source-ranged diagnostic.

## Source text and lines

The input is a complete `QString`. Every token range is stored as `[begin, end)` in UTF-16 code-unit offsets. Lines are one-based and are normalized from `LF`, `CRLF`, or a standalone `CR` to one logical newline. The raw source lexeme is retained unchanged in each token.

Outside delimiters and implicit continuation contexts, a physical line ending emits `Newline`. Blank and comment-only lines do not change indentation. The lexer suppresses logical `Newline` while an opening parenthesis, bracket, or brace has not been closed. The parser therefore receives a Python-like layout stream, which matches the indentation-delimited suites in the corpus.

## Whitespace, comments, and layout

Horizontal spaces and tabs are trivia. A comment begins with `#` outside a string and extends to, but does not include, the physical line ending. Comments are emitted on the trivia channel and never alter indentation.

Leading indentation is measured after a logical newline that begins a nonblank, noncomment line. The lexer maintains an indentation stack initialized to width zero. A wider indentation emits `Indent`; a smaller indentation emits one `Dedent` for each matching prior stack level. A width that does not match a prior level emits `LEX004` and recovers by treating the current width as a new stack level. The current baseline accepts tabs and spaces, measuring a tab as four columns; a future language decision may prohibit mixed indentation.

The Arabic semicolon `؛` is a parser-visible same-line statement separator. The initial implementation recognizes ASCII `;` as the same token for editor and migration tolerance; the language owner should decide whether ASCII semicolons remain accepted.

| Construct | Token behavior | Corpus examples |
|---|---|---|
| Horizontal whitespace | `Trivia` only | Calls and assignments throughout the corpus |
| `# comment` | One comment-trivia token | Lines 1–5 and inline examples |
| Physical newline | `Newline` outside open grouping delimiters | Lines 1–2 |
| Indented suite | `Newline`, `Indent`, tokens, `Dedent` | `اذا` suite at lines 266–271 |
| Same-line statements | `ArabicSemicolon` | `س = 5؛ ص = 9` at line 5 |
| Implicit continuation | Suppress `Newline` inside `()`, `[]`, `{}` | Calls, collections, and multiline structures |

## Identifiers and keywords

An identifier starts with `_` or a Unicode letter and continues with `_`, Unicode letters, or Unicode decimal digits. This includes the Arabic identifiers and magic names such as `__تهيئة__` in the corpus. The lexer preserves raw spelling and does not normalize Unicode. Member names after `.` are still ordinary identifiers.

Keywords are recognized only when the whole identifier matches an approved spelling. Keyword aliases share one `TokenKind` while retaining the exact source lexeme. Built-in functions, user functions, class names, `هذا`, and magic methods are identifiers at the lexical layer; only parser or semantic analysis may assign contextual meaning.

| Canonical kind | Accepted spellings in baseline | Corpus evidence |
|---|---|---|
| `KwFunction` | `دالة` | Lines 303 and 311 |
| `KwClass` | `صنف` | Lines 338 and 377 |
| `KwIf` | `اذا`, `إذا` | Line 266 |
| `KwElseIf` | `اواذا`, `أوإذا` | Line 268 |
| `KwElse` | `والا`, `وإلا` | Lines 270 and 298 |
| `KwFor` | `لكل` | Line 274 |
| `KwIn` | `في` | Lines 162 and 486 |
| `KwWhile` | `بينما` | Line 283 |
| `KwTry` | `حاول` | Lines 41 and 293 |
| `KwExcept` | `خلل` | Lines 44 and 296 |
| `KwFinally` | `نهاية` | Line 300 |
| `KwReturn` | `ارجع` | Lines 316 and 346 |
| `KwImport` | `استورد` | Lines 168 and 192 |
| `KwFrom` | `من` | Lines 170 and 203 |
| `KwDelete` | `احذف` | Line 35 |
| `KwBreak` | `توقف` | Lines 286 and 315 |
| `KwContinue` | `استمر` | Line 286 |
| `KwAnd` | `و` | Line 13 |
| `KwOr` | `او`, `أو` | Line 13 |
| `KwNot` | `ليس` | Lines 13 and 486 |
| `KwLambda` | `خطية` | Lines 321 and 324 |
| `KwTrue` | `صح` | Lines 39 and 123 |
| `KwFalse` | `خطأ`, `خطا` | Line 278 |
| `KwNull` | `عدم` | Lines 64 and 352 |

The existing language-definition list contains additional words such as `متوقع`, `مزامنة`, `انتظر`, `هل`, `مرر`, `عند`, and `انتج`. They are intentionally not finalised here because `Status.alif` does not demonstrate their syntactic roles. Add them to this table before assigning a dedicated lexical kind.

## Literals

Integers begin with ASCII digits. Decimal integers and decimals use ASCII digits, with an optional fractional part and exponent. Hexadecimal integers start with `0x` or `0X` and require at least one hexadecimal digit. Unary `+` and `-` are separate operator tokens, not part of a numeric token. Arabic digits, binary/octal forms, numeric separators, and special number spellings are not part of this baseline until approved.

A quoted string starts with `'` or `"`. A run of three matching quote characters starts a multiline string. Single-line strings cannot cross an unescaped physical line ending; multiline strings may. A backslash consumes the following code unit as escaped content. The runtime-specific meaning of `\س` and other escapes is outside lexical scanning; malformed or unterminated string structure still produces lexer diagnostics.

A formatted string begins with `م` immediately followed by an approved single or triple quote delimiter. It emits `FStringStart`, zero or more `FStringText` or interpolation token sequences, and `FStringEnd`. `{{` and `}}` represent literal braces. A single unescaped `{` emits `InterpolationStart`; expression lexing then proceeds normally until the matching interpolation `}` emits `InterpolationEnd`. Brace nesting is supported. An opening quote inside the interpolation starts an ordinary nested string. A colon within an interpolation remains a `Colon` token in this baseline; the parser will define format-spec parsing.

| Literal form | Tokenization | Corpus examples |
|---|---|---|
| Integer | `IntegerLiteral` | `9`, `-3`, `50` |
| Decimal | `FloatLiteral` | `3.14`, `0.5`, `1.6` |
| Hexadecimal | `IntegerLiteral` | Supported by prior lexer; add corpus fixture when used |
| Quoted string | `StringLiteral` | Lines 38–39 and 88–120 |
| Triple string | `StringLiteral` across lines | Lines 47–50 and 173–187 |
| Formatted string | `FStringStart`, structured body, `FStringEnd` | Lines 90, 236, 387, and 438–479 |

## Punctuation and operators

The lexer uses longest-match scanning. Every punctuation mark and operator below is emitted as a distinct token. The parser decides precedence and semantics.

| Source spelling | Token kind | Corpus evidence |
|---|---|---|
| `(` `)` | `LParen`, `RParen` | Calls throughout |
| `[` `]` | `LBracket`, `RBracket` | Lists and slices at lines 62 and 113 |
| `{` `}` | `LBrace`, `RBrace` | Maps and sets at lines 138 and 153 |
| `,` | `Comma` | Calls and tuple values |
| `،` | `ArabicComma` | Lines 6, 129, and 236 |
| `.` | `Dot` | Members and imports |
| `:` | `Colon` | Suites, maps, slices, and f-string formats |
| `؛` `;` | `ArabicSemicolon` | Line 5 |
| `+` `-` `*` | `Plus`, `Minus`, `Star` | Arithmetic and unpacking |
| `**` | `DoubleStar` | Keyword argument unpacking at line 329 |
| `^` | `Power` | Line 9 |
| `\` | `Slash` | Arithmetic at line 9 |
| `\*` | `StarSlash` | Arithmetic at line 9 |
| `\\` | `DoubleSlash` | Arithmetic at line 9 |
| `=` | `Equal` | Assignments throughout |
| `+=` `-=` `*=` | Compound assignment kinds | Lines 21–23 |
| `\=` `\*=` `\\=` `^=` | Compound assignment kinds | Lines 24–27 |
| `==` `!=` `<` `<=` `>` `>=` | Comparison kinds | Line 13 |

## Diagnostics and recovery

The lexer issues stable diagnostic codes and keeps scanning after an error.

| Code | Condition | Recovery |
|---|---|---|
| `LEX001` | Unexpected character | Emit `Invalid`, consume one code unit. |
| `LEX002` | Unterminated single-line string | Emit `Invalid` for the unterminated literal and resume after its physical line. |
| `LEX003` | Unterminated multiline or formatted string | Emit `Invalid` through end of source and then `EndOfFile`. |
| `LEX004` | Inconsistent indentation | Report at first nonmatching indentation character and continue with a fresh level. |
| `LEX005` | Malformed hexadecimal literal | Emit `Invalid` for the malformed literal span and continue. |
| `LEX006` | Unclosed formatted-string interpolation | Emit `Invalid` through the containing literal end or EOF. |

## Change protocol

Change this document before changing token kinds or lexical behavior. Each change must add at least one focused lexer fixture and, where applicable, one `Status.alif` corpus regression assertion. This document is the grammar reference for the parser, highlighter adapter, and future automated agents.
