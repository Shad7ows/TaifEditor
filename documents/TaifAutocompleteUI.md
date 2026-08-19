# Taif Rich Autocomplete UI

**Status:** Rich RTL popup restored and extended for semantic completion.  
**Scope:** `TCompletionPopup`, `TModernCompletionDelegate`, `CompletionModel`, and semantic completion item metadata.

## Design contract

The autocomplete experience uses one RTL `QCompleter` popup with a compact list, a right-side typed glyph, a right-edge selected-row accent, and a simple documentation footer. The popup remains entirely GUI-thread owned; semantic analysis supplies only immutable completion item data.

| Completion source | Icon | Category | Colour |
|---|---|---|---|
| Keyword | `{}` | `محجوزة` | Purple |
| Snippet | `<>` | `كتلة` | Red |
| Builtin | `()` | `ضمنية` | Green |
| Dynamic word | `أب` | `نص` | Blue |
| Semantic function/method | `()` | `دالة` | Blue |
| Semantic class | `[]` | `صنف` | Amber |
| Semantic field/property | `::` | `خاصية` | Teal |
| Semantic parameter | `@` | `معامل` | Purple |
| Semantic local | `أب` | `متغير` | Neutral |
| Semantic loop variable | `#` | `متغير حلقة` | Green |
| Semantic import | `->` | `اسم مستورد` | Orange |
| Semantic builtin | `()` | `مدمج` | Green |

## Implementation rules

`CompletionVisual` and `completionVisual()` centralise the icon, category, and colour mapping. Both the delegate and footer use this one mapping; this prevents an enum extension from rendering a row with uninitialised colour/icon state. The mapping covers all supported values and has a safe fallback for unknown values.

`CompletionItem` has `CompletionSemanticKind`, an UI-only subtype that intentionally does not include semantic-model headers. `SemanticCompletionProvider` maps language `SymbolKind` into it for normal and member completion paths. This lets `تويوتا.` distinguish methods from fields without inferring from labels or Arabic prose.

The footer escapes all documentation with `toHtmlEscaped()` before line-break rendering. It keeps RTL direction and a Tajawal/sans-serif fallback. Empty descriptions fall back to the mapped category rather than displaying an empty panel.

## Validation record

| Target | Result |
|---|---|
| Autocomplete/analysis test target | **9 passed, 0 failed**. Covers legacy/semantic/fallback visual mapping, model semantic-kind role, member field metadata, and existing tier/member regressions. |
| Full Qt application | Reconfigured and rebuilt successfully with the rich popup implementation. |

## Manual acceptance checklist

Verify the popup using a keyword, a snippet, a builtin, an in-scope semantic symbol, `سيارة.`, and `تويوتا.`. Each row should have a visible right-side glyph and icon colour, a selected right-edge accent, RTL primary label, and concise Arabic footer that updates with arrow-key selection. Unknown/future item types must show the neutral fallback rather than blank or unstyled content.
