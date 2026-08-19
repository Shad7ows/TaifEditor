# Taif Lexer Implementation Plan

**Status:** Foundation implemented on the current branch; parser and highlighter-adapter phases remain planned.  
**Scope:** Qt 6 / C++17 parser-ready lexer, with a compatible syntax-highlighter adapter  
**Primary syntax corpus:** `Status.alif`  
**Repository target after approval:** `documents/TaifLexerPlan.md`

## 1. Goal

Build a **language-core Taif lexer** that transforms a complete `.alif` source file into a deterministic, lossless, parser-ready token stream. The parser will consume this stream directly to create an AST; the existing Qt syntax highlighter will be refactored into a separate adapter that only maps lexical or later semantic categories to visual formats.

The current `TLexer` is useful as a highlighter scanner, but it is not yet a reliable parser boundary. It accepts a single `QTextDocument` block and an integer continuation state, returns theme-oriented token types, uses call/class-name heuristics, and has no explicit end-of-file token or lexical diagnostic result. The new design must preserve editor responsiveness while making the parser independent of `QSyntaxHighlighter` and its compact per-block state model.

> **Core contract:** `QString` source in; ordered grammar tokens, exact source ranges, and lexical diagnostics out. The output always ends in `EndOfFile`.

## Implementation record

| Plan area | Current state | Evidence |
|---|---|---|
| Grammar and living-plan artifacts | Implemented | `documents/TaifLexicalGrammar.md` and this document are tracked in the repository. |
| Parser-facing token and lexer core | Implemented | `source/language/lexer/TaifLexer.h` and `.cpp`. |
| Status corpus and focused tests | Implemented | `tests/lexer/tst_TaifLexer.cpp` and `tests/lexer/data/Status.alif`. |
| C++17 project integration | Implemented | `taif/Taif.pro` compiles the core lexer with C++17. |
| Parser baseline and AST handoff | Implemented | `source/language/parser/`, `documents/TaifParserPlan.md`, and `tests/parser/`. |
| Semantic scope and symbol model baseline | Implemented | `source/language/semantic/`, `documents/TaifSymbolTablePlan.md`, and `tests/semantic/`. |
| Localized incremental checkpoints, semantic reuse, and highlighter adapter | Not started | These remain the next editor-performance milestones. |

The implemented baseline includes source ranges, main/trivia channels, layout tokens, recognized corpus keywords, exact operator tokens, quoted/triple strings, formatted-string interpolation, and a raw `FStringFormat` token after an interpolation colon. The parser must still define the format-spec mini-language.

## 2. Evidence gathered from `Status.alif`

`Status.alif` is the primary working corpus for the first lexer specification. It confirms that the language is Python-like in its layout and expression syntax, while using Arabic keywords and supporting a mix of Arabic and ASCII punctuation. The corpus is an implementation guide and test corpus; it is not treated as a complete formal grammar. Where its behavior is unclear, the language specification must decide the rule before implementation is finalized.

| Lexical area | Corpus evidence | Initial lexer implication |
|---|---|---|
| Comments | `#` at line start and after statements | Emit comment trivia from `#` to the physical line end. |
| Statements and layout | Newlines, tab-indented suites after `:`, and Arabic semicolon `؛` for multiple statements on one line | Emit `Newline`; implement an indentation stack and `Indent`/`Dedent`; emit `ArabicSemicolon` as a statement separator. Decide whether ASCII `;` is also valid. |
| Identifiers | Arabic names, underscore, magic names such as `__تهيئة__`, and members after `.` | Support Unicode Arabic identifiers with `_`; tokenize magic names as identifiers, not a distinct parser terminal. |
| Keywords | Examples include `اذا`, `اواذا`, `والا`, `لكل`, `في`, `بينما`, `حاول`, `خلل`, `نهاية`, `دالة`, `صنف`, `ارجع`, `استورد`, `من`, `خطية`, `ليس` | Emit precise keyword kinds, with an explicit alias/canonicalization table for accepted spellings. |
| Literals | Integers, decimals, `0x` currently in lexer rules, strings, triple strings, lists, tuples, maps, sets, booleans and null-like values | Implement explicit literal categories and preserve raw lexemes/ranges. Confirm all numeric bases and textual literal spellings. |
| Strings | `'...'`, `"..."`, `"""..."""`, Arabic escape forms such as `\س`, and escaped content | Specify escape rules; distinguish single-line and multiline literals; diagnose unterminated literals. |
| Formatted strings | `م"...{expression}..."`, nested quote use inside interpolation, and format-spec examples such as `{س:.2ف}` | Emit f-string structure (`FStringStart`, text, interpolation boundaries, `FStringEnd`) and retain nested brace state. Decide whether format specification is handled lexically or by the expression parser. |
| Operators | Arithmetic `+ - * ^ \ \* \\`; comparisons; logical words; assignment and compound assignment; unpacking `*` and `**` | Use a longest-match token table with every legal operator as its own token kind. |
| Punctuation | Both `,` and `،`, `()[]{}`, `.`, `:`, `=` and `؛` | Explicitly support and test both comma spellings if both are official; keep distinct lexemes while normalizing only if the grammar allows equivalence. |
| Imports | Dotted and relative imports such as `.`, `..`, and module members | Emit dots as punctuation; parser decides import path grammar. |

## 3. Target lexer boundary

The parser-facing lexer will be a Qt Core-only component. It will not include `QSyntaxHighlighter`, theme classes, `QTextDocument`, or widget code. It may use `QString`, `QStringView`, `QVector`, and `QHash`, and will remain C++17 compatible.

```cpp
struct SourceLocation final {
    qsizetype offset = 0;  // UTF-16 offset into the full QString
    qsizetype line = 1;    // 1-based physical line
    qsizetype column = 1;  // 1-based UTF-16 column
};

struct SourceRange final {
    SourceLocation begin;
    SourceLocation end;    // exclusive
};

enum class TokenChannel : quint8 { Main, Trivia };

enum class TokenKind : quint16 {
    EndOfFile,
    Invalid,

    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    Newline, Indent, Dedent,

    // Exact Taif keywords; the final list comes from the reviewed grammar.
    KwFunction, KwClass, KwIf, KwElseIf, KwElse, KwFor, KwIn,
    KwWhile, KwTry, KwExcept, KwFinally, KwReturn,
    KwImport, KwFrom, KwDelete, KwBreak, KwContinue,
    KwAnd, KwOr, KwNot, KwIsNot, KwLambda,
    KwTrue, KwFalse, KwNull,

    LParen, RParen, LBracket, RBracket, LBrace, RBrace,
    Comma, ArabicComma, Dot, Colon, ArabicSemicolon,

    Plus, Minus, Star, DoubleStar, Slash, StarSlash, DoubleSlash,
    Equal, PlusEqual, MinusEqual, StarEqual, SlashEqual,
    StarSlashEqual, DoubleSlashEqual, PowerEqual,
    EqualEqual, NotEqual, Less, LessEqual, Greater, GreaterEqual,

    FStringStart, FStringText, InterpolationStart,
    InterpolationEnd, FStringEnd
};

struct Token final {
    TokenKind kind = TokenKind::Invalid;
    TokenChannel channel = TokenChannel::Main;
    SourceRange range;
    QString lexeme;        // Raw source spelling; optimize only after profiling.
};

struct LexDiagnostic final {
    QString code;          // Example: LEX001
    QString message;
    SourceRange range;
};

struct LexResult final {
    QVector<Token> tokens;             // Always has one final EndOfFile token.
    QVector<LexDiagnostic> diagnostics;
};

class TaifLexer final {
public:
    [[nodiscard]] LexResult lex(const QString& source) const;
};
```

All ranges use half-open UTF-16 positions (`[begin, end)`). This matches `QString`, `QTextDocument`, and Qt text-selection coordinates, avoiding conversion errors between parser diagnostics and editor underlines. Any UI that needs display columns can calculate them from the retained source text without changing parser ranges.

The lexer will preserve comments and non-structural whitespace on a `Trivia` channel. The parser consumes only the main channel, while a formatter, future lossless syntax tree, code actions, and highlighter adapter can retain exact source layout. `Newline`, `Indent`, and `Dedent` remain main-channel tokens because the corpus demonstrates indentation-delimited suites.

## 4. Grammar decisions to record before implementation

The corpus demonstrates syntax but cannot answer every edge case. A short, versioned lexical grammar must be added before the lexer is considered complete. It should record positive and negative examples for every decision below.

| Decision required | Why it matters | Proposed default pending approval |
|---|---|---|
| Keyword aliases | Existing lexer lists variants such as `اذا` and `إذا`, and `اواذا` / `أوإذا`. | Accept the current aliases, emit one canonical keyword kind, retain raw spelling. |
| Indentation policy | The corpus uses tab-indented blocks. | Allow tabs initially; define whether spaces are valid and whether mixed indentation is an error. |
| Newline policy | Needed for statements, blank/comment lines, and bracket continuations. | Emit one logical `Newline` outside `()`, `[]`, and `{}`; suppress it in implicitly continued expressions. |
| Semicolon policy | The corpus contains Arabic `؛` between same-line statements. | Emit it explicitly; confirm whether ASCII `;` is valid or rejected. |
| Arabic punctuation equivalence | The corpus uses both `,` and `،`. | Recognize both; require parser grammar to decide whether they are equivalent separators. |
| Operator meanings and spellings | `\`, `\*`, and `\\` appear in arithmetic and compound assignment. | Tokenize every spelling exactly by longest match; parser/runtime defines semantic meaning and precedence. |
| Numeric literal grammar | Current code supports decimal and hexadecimal only. | Confirm bases, exponent notation, leading/trailing decimal dots, numeric separators, Arabic digits, and invalid literal recovery. |
| Identifier policy | Arabic and underscore identifiers appear throughout. | Define accepted Unicode categories, normalization behavior, and whether non-Arabic identifiers are supported. |
| Escape sequences | The corpus shows Arabic `\س` and ordinary escaped string content. | Document exact escape table and invalid-escape diagnostics. |
| F-string format syntax | Format specs follow `:` inside `{...}`. | First parse the interpolation expression structurally; decide whether the lexer emits a dedicated format-spec mode or treats its content as a parser-managed subgrammar. |
| Literal prefix grammar | `م"..."` appears as the format-string prefix. | Allow exactly the reviewed prefixes and delimiters; do not infer unapproved repeated prefixes from the old highlighter behavior. |
| Error recovery | Parser errors must be stable after malformed input. | Emit `Invalid` tokens plus diagnostics, always advance the cursor, and resume at a safe lexical boundary. |

## 5. Architecture and file layout

After approval, source changes will introduce a new language layer rather than alter the current highlighting types in place.

```text
source/
  language/
    lexer/
      TaifLexer.h
      TaifLexer.cpp
      Token.h
      SourceLocation.h
      LexerDiagnostic.h
      LanguageSpec.h
      LexerCheckpoint.h            # Editor-only incremental continuation state
  texteditor/
    highlighter/
      TLexerAdapter.h/.cpp         # Maps parser tokens to visual token categories
      TSyntaxHighlighter.*         # Uses the adapter, not language-core heuristics

tests/
  lexer/
    tst_TaifLexer.cpp
    data/
      Status.alif
      token_expectations/          # Small focused fixtures

documents/
  TaifLexerPlan.md                 # This living plan after approval
  TaifLexicalGrammar.md            # Formalized grammar decisions
```

The language-core lexer must own all mutable cursor and mode state per `lex()` invocation. It must not retain a `finalState` member that leaks state between calls. A future incremental editor API may use a typed `LexerCheckpoint`, but this checkpoint is not part of the parser’s normal full-document API.

```text
full .alif QString
        |
        v
+-------------------------------+
| TaifLexer (language core)     |
| cursor, state stack, layout   |
+-------------------------------+
        |                 |
        v                 v
  QVector<Token>    QVector<LexDiagnostic>
        |
        v
     TaifParser
        |
        v
       AST

Editor highlighter --> incremental lexer adapter --> TokenKind-to-theme mapping
```

For the editor, complete state must be retained when a construct crosses a `QTextDocument` block. Qt's integer block state remains suitable as an invalidation marker, but it cannot reliably encode f-string brace depth, nested literal state, and an indentation stack. The adapter will store a full typed checkpoint per block through `QTextBlockUserData` or a document-revision-aware block cache. Qt documents both the per-block state mechanism and custom block user data for cached parsing information. [1]

## 6. Step-by-step implementation plan

### Phase 0 — Formalize the lexical grammar

Create `documents/TaifLexicalGrammar.md` from the `Status.alif` corpus. It will include the complete keyword map, aliases, punctuation, operators, literal forms, comments, line continuation, indentation rules, and error behavior. Each grammar entry will cite one or more corpus examples and include an invalid counterpart. The unresolved decisions in Section 4 must be reviewed before moving forward.

**Exit criterion:** The language owner approves a lexical grammar table and at least one minimal `.alif` program that exercises each terminal family.

### Phase 1 — Create the parser-facing token, range, and diagnostic types

Add the language-core data types shown in Section 3. Configure production sources to remain C++17 compatible; the current project file requests C++23, so this setting will be deliberately reviewed and changed only if no non-lexer feature depends on C++23. Provide constexpr-safe helpers for source ranges and token predicates.

**Exit criterion:** A small test lexes an empty file and simple assignment, producing exact ranges and exactly one final `EndOfFile` token.

### Phase 2 — Build deterministic cursor and basic scanner routines

Implement a private scanner context with `peek`, `advance`, `match`, range construction, line/column tracking, and a strict progress invariant. Add scanning in this order: comments/trivia, physical lines and indentation measurement, identifiers, keywords, numbers, punctuation, and longest-match operators. Emit `Invalid` with a diagnostic for unsupported characters rather than classifying all unknown text as a generic operator.

Do not use function-call or PascalCase heuristics. An identifier followed by `(` remains an `Identifier`; the parser will recognize a call expression. A class/function name remains an `Identifier`; the parser will recognize it in a declaration grammar.

**Exit criterion:** All simple `Status.alif` lines through the arithmetic, comparison, assignment, collection, import, and block samples tokenize with expected terminal kinds and no scanner stalls.

### Phase 3 — Implement strings, f-strings, and structured recovery

Implement ordinary quoted strings, triple-quoted multiline strings, approved escape sequences, and diagnostics for unterminated literals. Then implement formatted strings as a token sequence: opening prefix/delimiter, text chunks, interpolation openings, normal expression tokens, interpolation closings, and final delimiters. Maintain a typed state stack so nested braces, nested quoted strings inside interpolation, escaped braces, and triple formatted strings work correctly.

The format-spec construct after `:` in an interpolation will be implemented according to the approved lexical grammar. If it has language-specific mini-language rules, introduce a dedicated f-string-format token/mode instead of pretending it is an ordinary expression.

**Exit criterion:** Focused tests cover every formatted-string pattern in `Status.alif`, including the nested-quote example around line 236, format specs around lines 438–479, and malformed strings/braces.

### Phase 4 — Add and enforce layout tokens

Implement physical-to-logical newline normalization and an indentation stack. Ignore indentation changes on blank/comment-only lines, handle line continuations defined by the grammar, emit one or more `Dedent` tokens when indentation decreases, and flush remaining `Dedent` tokens before `EndOfFile`. Report inconsistent indentation using stable error codes and source ranges.

**Exit criterion:** The `اذا`, `لكل`, `بينما`, `حاول`, function, and class suites in `Status.alif` produce stable `Newline`/`Indent`/`Dedent` sequences.

### Phase 5 — Introduce the parser hand-off contract

Create the parser module only after Phases 1–4 are stable. Its public entry point receives the core token sequence and diagnostic sink/result. It must ignore trivia centrally, consume exact `TokenKind` values, and create AST nodes with source ranges. The parser must not depend on `TToken`, `TokenType`, `TSyntaxHighlighter`, or theme code.

**Exit criterion:** Parser smoke tests successfully parse a basic module containing assignments, a conditional suite, a function declaration, a class declaration, an import, and an f-string expression; malformed source reports source-ranged errors without a crash.

### Phase 6 — Migrate highlighter integration

Add `TLexerAdapter` to convert `TokenKind` into existing visual categories. All `Kw*` kinds map to the current keyword style; string families and comments map to their current formats; identifiers stay identifiers unless future parser/semantic analysis provides an accurate contextual category. Remove or retire the current state-machine `TLexer` only when style regression tests pass.

For per-block highlighting, the adapter will use a full `LexerCheckpoint` and safely re-lex affected blocks after edits. Full-document parser lexing remains the source of truth.

**Exit criterion:** Existing themes preserve expected highlighting while parser tokenization remains untouched by UI behavior.

### Phase 7 — Harden performance and recovery

Add corpus regression tests using the complete `Status.alif` file, fuzz/property tests for arbitrary Unicode input, test cases for malformed nested f-strings and invalid indentation, and a benchmark that measures lexing time and token allocation count for the corpus. Keep every scanning loop terminating and every invalid character consuming at least one code unit.

**Exit criterion:** No hangs, no zero-progress loops, deterministic diagnostics, and acceptable editor re-highlighting performance on the corpus.

## 7. Test strategy

Use a dedicated Qt Test target such as `tests/lexer/tst_TaifLexer.cpp`. Each expected token test record must state its kind, source text, channel, start/end offset, start/end line/column, and diagnostic code when applicable. Tests must exercise the language-core lexer directly, never through a GUI highlighter.

| Test family | Representative `Status.alif` coverage | Required assertion |
|---|---|---|
| Ranges and newlines | First comment/assignment lines; LF and CRLF variants | Half-open UTF-16 ranges and line/column values are exact. |
| Statement separators | Multiple statements using `؛`; regular newline statements | Parser-visible statement boundaries are emitted precisely. |
| Arabic/ASCII punctuation | Calls and lists using both `,` and `،` | Both spellings follow the approved grammar policy and preserve raw lexeme text. |
| Operators | Arithmetic, comparisons, compound assignments, star forms | Longest-match scanning never splits a valid multi-character operator. |
| Identifiers and keywords | Functions, classes, imports, magic methods | Aliases map to approved keyword kinds; all non-keyword names remain identifiers. |
| Collections and imports | Lists, tuples, maps, sets, dotted and relative imports | Every delimiter is an exact token; parser grammar can reconstruct nesting and paths. |
| Layout | `اذا`, loops, exception blocks, nested class/function suites | Correct `Newline`, `Indent`, `Dedent`, including EOF dedent flush. |
| Strings | Ordinary, triple-quoted, Arabic escapes | Boundaries and diagnostics are correct. |
| Formatted strings | Basic interpolation, nested mapping lookup, format specifiers | Structured f-string tokens preserve nested braces and resumable state. |
| Invalid input | Unknown character, malformed operator, unterminated string, unmatched braces, invalid indentation | A diagnostic is produced and lexing progresses to EOF. |
| Corpus regression | Whole `Status.alif` file | Tokenization is deterministic; the expectation snapshot changes only through reviewed grammar updates. |

## 8. Risks and mitigation

| Risk | Impact | Mitigation |
|---|---|---|
| Treating `Status.alif` as a complete formal specification | Hidden language features may be missed. | Create and approve a versioned grammar document; add new source samples as regression fixtures. |
| Mixing parser and highlighting token models | Parser behavior becomes dependent on style heuristics. | Maintain separate core tokens and a one-way highlighter adapter. |
| Losing nested state in block highlighting | Incorrect styling after multi-block f-strings or edits. | Use typed checkpoints stored per block; do not serialize full state into an `int`. |
| Ambiguous punctuation/aliases | Parser disagreements and inconsistent source acceptance. | Decide and document exact accepted spellings before tests are locked. |
| Unicode coordinate ambiguity | Incorrect diagnostics and editor underlines. | Declare UTF-16 source ranges as canonical and test Arabic/non-BMP input. |
| Incremental performance regression | Slow highlighting in large documents. | First verify full lexer correctness; then benchmark/checkpoint incremental adapter separately. |

## 9. Living-plan maintenance

Once approved, this document will be added to the repository at `documents/TaifLexerPlan.md`. It will be maintained as the reference plan for engineers and future agents.

Every plan change must update the **Status**, **Decision log**, **phase completion criteria**, and relevant tests. Grammar changes must first update `documents/TaifLexicalGrammar.md` and add a minimal regression example before changing lexer code. No phase is considered complete until its exit criterion and associated tests pass.

| Decision | Status | Owner | Evidence / follow-up |
|---|---|---|---|
| Use `Status.alif` as the initial lexer corpus | Accepted for planning | Language/Editor team | Convert focused examples into test fixtures in Phase 0. |
| Treat Taif as indentation-sensitive | Proposed; corpus-backed | Language owner | Approve exact tabs/spaces and continuation rules. |
| Use parser-core tokens rather than highlighter token types | Proposed | Architecture owner | Implement in Phase 1. |
| Use UTF-16 canonical source offsets | Proposed | Parser/Editor team | Validate with Arabic and non-BMP test cases. |
| Support Arabic comma and Arabic semicolon | Proposed; corpus-backed | Language owner | Confirm equivalence rules and ASCII alternatives. |
| F-string format-spec lexical design | Baseline implemented | Language owner / parser owner | Lexer emits `Colon` followed by raw `FStringFormat`; parser must define the mini-language. |

## 10. Definition of done

The lexer work is complete when the core lexer, invoked on a complete `.alif` source, returns an ordered stream of exact grammar tokens ending in `EndOfFile`, reports lexical diagnostics with reliable ranges, handles all approved `Status.alif` syntax, and passes deterministic unit, corpus, malformed-input, and performance tests. The parser consumes only this stream. The Qt highlighter remains a separate presentation adapter that can incrementally re-lex document blocks without redefining language semantics.

## References

[1] [QSyntaxHighlighter Class — Qt 6](https://doc.qt.io/qt-6/qsyntaxhighlighter.html)
