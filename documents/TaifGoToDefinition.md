# Go to Definition

**Status:** Implemented and validated  
**Applies to:** Qt 6 / C++17 TaifEditor semantic navigation

## Purpose

TaifEditor now supports precise, local-document go-to-definition from the immutable Tier 2 language-analysis snapshot. The feature navigates from a resolved symbol use to the exact declaration-name range without reparsing the document or guessing from text boundaries.

> **Navigation invariant:** A target is valid only when the active editor revision matches the immutable analysis snapshot, the initiating offset is an identifier token, semantic resolution identifies one canonical local symbol, and that symbol’s declaration range is a valid identifier range in the current document.

## User interaction

| Trigger | Behavior |
|---|---|
| `F12` | Navigates from the current editor cursor to the resolved declaration. |
| `Ctrl+Left Click` | Navigates from the clicked identifier; a non-resolved click retains normal text-editor behavior. |
| Context menu | Offers the RTL action `اذهب إلى التعريف`, enabled only for a valid target. |
| `Alt+Left` | Returns to the prior local navigation position when history is available. |

A successful navigation selects the declaration name and scrolls it into view. Completion and hover transient UI are dismissed first so the destination is visually unambiguous.

## Architecture

`SemanticDefinitionProvider` is a Qt Core-only adapter over `LanguageAnalysisSnapshot`. It receives a UTF-16 offset and returns an immutable `DefinitionLocation` with the source token range, canonical symbol ID/name, declaration range, and one-based declaration line. It does not access `QTextDocument`, widgets, or mutable editor state.

`TEditor` validates that the controller’s current snapshot revision equals the active document revision, then applies the result to a fresh `QTextCursor`. The editor keeps a bounded 64-entry local navigation history as raw anchor/position offsets. History is cleared on document edits so old offsets cannot be reapplied to changed text.

| Component | Responsibility |
|---|---|
| `SemanticDefinitionProvider` | Exact snapshot-safe offset-to-definition resolution. |
| `DefinitionLocation` | Immutable target range/value contract. |
| `TEditor` | F12, Ctrl+Click, context-menu action, cursor selection, scrolling, and `Alt+Left` history. |
| `SemanticModel` | Existing references, resolved symbols, declaration ranges, scopes, and member resolution. |

## Resolution policy

The provider first checks that the source offset lies in a main-channel `Identifier` token. It then uses `SemanticModel::referenceAt()` for resolved use sites; where no use-site reference exists, it selects the narrowest declaration range containing the offset. This allows the command to be used consistently on both a reference and a declaration name.

| Target state | Result |
|---|---|
| Resolved function/class/local/parameter/loop variable | Navigate to the local declaration. |
| Resolved field or method member | Navigate to the canonical class member declaration. |
| Resolved local import binding | Navigate to the local `استورد` or `من … استورد …` declaration. |
| Declaration name | Select that same declaration. |
| Blank line, whitespace, punctuation, keyword, or string | No target. |
| Unresolved, ambiguous, external, malformed, or stale symbol | No target. |
| Builtin/prelude declaration without an in-document identifier range | No target. |

The current implementation intentionally limits navigation to verified in-document ranges. External module definitions and unknown remote members are not fabricated. Cross-file definition navigation will build on `DefinitionLocation` once import/module indexing can provide a trusted file and range.

## Regression coverage and validation

The analysis suite covers member-field navigation, import-binding navigation, declaration navigation, blank-token rejection, and stale-snapshot rejection. Existing semantic hover token eligibility is shared in spirit: no navigation begins from whitespace or an arbitrary cursor position after a symbol.

| Validation target | Result |
|---|---|
| Analysis / semantic navigation suite | Passed: 15 tests. |
| Full TaifEditor Qt 6.11.1 / MSVC 2022 build | Passed. |

## Future extension

The next navigation layers can add Ctrl-hover link affordances, a definition preview, multi-result UI for future overload/ambiguity support, cross-file opening through a project/module index, and find-references. They should retain the current revision/range checks and extend `DefinitionLocation` rather than bypassing semantic resolution in the editor UI.
