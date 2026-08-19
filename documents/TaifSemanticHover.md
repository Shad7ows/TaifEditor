# Semantic Hover Tooltip

**Status:** Implemented and validated  
**Applies to:** Qt 6 / C++17 TaifEditor semantic analysis and text-editor UI

## Purpose

The editor now provides a precise, RTL-first hover tooltip for symbols that resolve in the current immutable semantic snapshot. The popup presents a symbol’s semantic category, declaration line, and available documentation without launching any new analysis work from mouse movement.

> **Precision invariant:** A hover result is resolved from UTF-16 source offsets, `SemanticModel::referenceAt()`, declaration ranges, and resolved symbol IDs. It never guesses from regexes or word boundaries.

## Architecture

The feature deliberately preserves the language-core boundary. `SymbolTableBuilder` remains independent of source strings and widgets. The editor supplies source text only to the pure hover adapter for documentation extraction, while semantic identity comes from the immutable `LanguageAnalysisSnapshot`.

| Component | Responsibility |
|---|---|
| `SemanticHoverProvider` | Resolves an offset to `HoverInfo`; returns category, declaration range/line, signature, and documentation. |
| `HoverInfo` | Immutable C++17 value object shared from provider to UI. All ranges use UTF-16 half-open offsets. |
| `THoverPopup` | Non-focusable dark RTL tooltip surface styled to complement autocomplete. |
| `TEditor` | Debounces pointer movement, validates snapshot/current revision, positions the popup, and handles dismissal lifecycle. |

## Resolution policy

A hover query is eligible only when the current editor revision and `LanguageAnalysisSnapshot` revision match. It first asks `SemanticModel::referenceAt(offset)` for a resolved use. If there is no reference, it selects the narrowest declaration range containing the offset so declaration names themselves can be hovered.

| Hover target | Result |
|---|---|
| Resolved name or call | The referenced canonical semantic symbol. |
| Resolved member name | The resolved member `Field` or `Function`, not its receiver. |
| Declaration name | The symbol declared at that exact range. |
| Unresolved, ambiguous, external-only, or recoverable-error symbol | No tooltip. |
| Stale snapshot | No tooltip. |
| Imported module or imported name | The resolved local import binding, its declaration line, and verified import provenance. |

The current baseline has semantic categories rather than general type inference. The **Type** field therefore reports an accurate category such as `دالة`, `صنف`, `خاصية`, `معامل`, or `متغير محلي`. It does not invent runtime/static types that are not supported by the language model yet.

## Documentation policy

Documentation is extracted in the editor-facing provider, not in the symbol table. It follows a deterministic priority order.

| Priority | Source | Rule |
|---:|---|---|
| 1 | Imported binding provenance | For `استورد …` and `من … استورد …`, show the exact local import declaration from the source snapshot. Remote API details are not fabricated. |
| 2 | Function/class docstring | The first direct string-literal expression in the declaration suite. |
| 3 | Preceding comments | Contiguous `#` comment tokens immediately preceding the declaration at matching indentation. |
| 4 | Fallback | `لا توجد وثائق متاحة`. |

Documentation is bounded to 12 lines and 800 UTF-16 code units for responsive rendering. A trailing ellipsis signals truncation. This avoids a large doc block causing disruptive tooltips while preserving useful source-context documentation.

## RTL visual system

`THoverPopup` intentionally uses the established autocomplete visual vocabulary while remaining a dedicated non-interactive tooltip.

| Region | Design |
|---|---|
| Header | Right-aligned Arabic semantic category and declaration/signature, with the **same centralized semantic colour and glyph mapping** as autocomplete. |
| Glyph zone | Fixed far-left icon zone; Arabic text remains in the right/main reading area, matching autocomplete row geometry. |
| Metadata | Compact RTL row showing `النوع` and exact one-based `التعريف: السطر N`. |
| Documentation | Wrapped RTL body using the same dark `#2c313a` footer treatment and blue divider vocabulary as autocomplete documentation. |
| Chrome | Autocomplete-compatible `#1e202e` surface, `#4b5263` border, and `#4793FF` top accent. |

The popup is non-focusable and transparent to mouse events. It is positioned below the hovered character where possible, then clamped to available screen geometry; it moves above the token if the lower screen edge has insufficient room.

## Lifecycle

Hover uses a 350 ms single-shot debounce timer. This avoids flicker during normal pointer movement and keeps expensive GUI work out of the mouse event itself.

| Event | Behavior |
|---|---|
| Pointer rests over a resolved target | Resolve from the current immutable snapshot and show/update the popup. |
| Pointer moves away, leaves the editor, or lands on whitespace/unresolved syntax | Hide the popup and cancel pending hover. |
| Document edit or revision change | Hide immediately; stale data is never displayed. |
| Scroll, keyboard activity, focus loss, or Escape | Hide immediately. |
| Autocomplete popup visible | Hover is suppressed to prevent overlapping transient surfaces. |

## Test coverage and validation

The analysis regression suite verifies resolved function hover, declaration line numbers, function docstrings, contiguous Arabic comment documentation for locals, unresolved-name rejection, and stale-snapshot rejection.

| Validation target | Result |
|---|---|
| Lexer Qt suite | Passed: 10 tests. |
| Parser Qt suite | Passed: 12 tests. |
| Semantic Qt suite | Passed: 12 tests. |
| Analysis / hover Qt suite | Passed: 14 tests. |
| Full TaifEditor build | Passed with Qt 6.11.1 and MSVC 2022. |

## Extension path

Future type inference can enrich `HoverInfo` with an optional inferred-type field while keeping the present category field intact. Module indexing can later support safe external import/member hover. More documentation sources, such as formal annotations or project API documentation, should be added above the current priority hierarchy without changing the popup lifecycle or semantic resolution contract.
