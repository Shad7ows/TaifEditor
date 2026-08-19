# Taif Class and Instance Member Completion

**Status:** Initial class-member semantic model and editor dot-completion support implemented.  
**Scope:** User-defined class methods and fields, direct constructor assignment inference, and completion after `ClassName.` or `instance.`.

## Supported semantic relationship

The semantic model now records three linked facts. A `Class` symbol owns a class scope; that scope contains declared `Function` methods and `Field` attributes; and a local/field assigned from a direct call to a known class receives an `instanceClass` symbol ID.

| Source pattern | Semantic result |
|---|---|
| `صنف سيارة:` | Declares class symbol `سيارة` and records its class scope. |
| `لون = 0` in class body | Declares a class `Field` named `لون`. |
| `دالة تغيير_لون_السيارة(هذا, …):` | Declares a class method. |
| `هذا.لون = لون` inside a class method | Declares or reuses field `لون`; `هذا` is associated with its enclosing class. |
| `تويوتا = سيارة()` | Declares/infer `تويوتا` as an instance of class `سيارة`. |
| `تويوتا.` / `سيارة.` | Resolves the receiver and offers class methods/fields through the semantic completion provider. |

The direct assignment inference is deliberately conservative. It recognizes only a single known class name called directly on the right-hand side. Factory functions, chained calls, returned instances, imports, inheritance, aliases, and flow-sensitive reassignment require later type analysis.

## Editor behavior

`TEditor` detects a simple identifier receiver immediately to the left of the current dot, including an empty member prefix. This means completion is valid as soon as the user writes:

```alif
تويوتا.
```

The semantic completion provider resolves the receiver from `visibleSymbolsAt(cursor)`, follows either the class symbol or the variable’s inferred `instanceClass`, and lists matching fields/methods. Static keyword/snippet suggestions are intentionally suppressed in member-access context so the popup contains member-relevant options only.

## Validation record

| Target | Result |
|---|---|
| Semantic model tests | **12 passed, 0 failed**; validates fields, methods, constructor inference, and class/instance member parity. |
| Analysis tests | **8 passed, 0 failed**; validates `SemanticCompletionProvider` for both `سيارة` and `تويوتا`. |
| Full Qt application build | Successful with semantic, completion-provider, and editor changes. |

## Future extensions

The next type-analysis milestones are constructor/factory return types, aliases, reassignment invalidation, inherited members, static versus instance members, visibility/decorators, imported classes, nested member chains such as `a.b.c`, indexed receivers, and type-aware signature/parameter help after selecting a method.
