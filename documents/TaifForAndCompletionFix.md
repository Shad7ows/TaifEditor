# Taif For-Loop and Immediate Completion Corrections

**Status:** Implemented and validated.  
**Scope:** Correct context-sensitive `لكل … في …:` parsing and remove legacy dynamic-word suggestions from the live editor popup.

## Correct `for` header grammar

The parser now treats `في` as a structural delimiter only while parsing a `for` or comprehension binding target. The generic Pratt expression grammar continues to treat `في` as a membership operator everywhere else.

| Context | Grammar treatment of `في` |
|---|---|
| `لكل ب في مدى(5):` | Structural delimiter between `ForTarget` and iterable expression. |
| `[ب لكل ب في عناصر]` | Structural delimiter between comprehension target and iterable expression. |
| `اذا ب في عناصر:` | Normal membership operator in a general expression. |

`parseBindingTarget()` currently accepts identifier targets and produces controlled `PAR005` recovery for unsupported targets without consuming the required `في` delimiter. This fixes the supplied loop:

```alif
لكل ب في مدى(5):
	اطبع(ب)
```

The symbol table records `ب` as a `LoopVariable`, resolves the iterable before binding that target, and resolves the body reference to the same symbol.

## Immediate semantic completion policy

The live popup no longer registers `DynamicWordStrategy`. It therefore never initially displays scope-blind regex-scanned words and later swaps them for semantic results.

| Popup state | Source of suggestions |
|---|---|
| Current semantic snapshot | Scope-aware visible symbols, then static keywords/builtins/snippets. |
| Stale semantic snapshot | Conservatively filtered module/prelude semantic symbols, then static providers; an immediate asynchronous completion analysis request is queued. |
| No semantic snapshot yet | Static providers from the new pipeline, while an immediate asynchronous completion analysis request is queued. |
| Current worker result arrives | The active popup refreshes through the current-revision semantic model. |

The completion request bypasses only the 300 ms **timer**. It still runs lexer, parser, and symbol-table work in the existing worker thread, is coalesced to one request per revision, and passes the same revision gate as normal Tier 2 highlighting. No language analysis was moved into the synchronous document-change handler.

## Validation record

| Target | Result |
|---|---|
| `TaifParserTests` | **12 passed, 0 failed**; includes valid `لكل ب في مدى(5):` parser regression. |
| `TaifSemanticTests` | **11 passed, 0 failed**; includes loop-variable binding/body resolution. |
| `TaifAnalysisTests` | **7 passed, 0 failed**; includes immediate user-initiated completion analysis before the 300 ms timer. |
| Full Qt application | Reconfigured and rebuilt successfully with parser/controller/editor changes. |

## Follow-up work

Tuple/list destructuring targets need explicit support in `parseBindingTarget()` before they are advertised as valid `for` targets. Completion can safely offer local symbols from a stale model only after a text-edit position mapper and scope-range reuse are implemented; until then the conservative stale path exposes only module and prelude declarations.
