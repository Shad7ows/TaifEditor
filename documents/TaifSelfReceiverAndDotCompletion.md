# Taif Dot-Safe Completion and `هذا` Receiver

**Status:** Implemented and validated.  
**Scope:** Completion acceptance after `receiver.`, self/instance receiver member lookup, and semantic highlighting of `هذا`.

## Dot-safe acceptance

Autocomplete no longer discovers the insertion range with `QTextCursor::PreviousWord` at acceptance time. That Qt word-boundary operation can include the receiver or its dot after `تويوتا.`, which was the cause of dot removal on Enter or Tab.

`CompletionContext` now records the exact UTF-16 member-prefix range when the popup is created. The editor stores it with the current analysis revision and uses that range for acceptance.

| Source before accepting a completion | Selected replacement range | Result for candidate `تغيير_لون_السيارة` |
|---|---|---|
| `تويوتا.` | Empty range after the dot | `تويوتا.تغيير_لون_السيارة` |
| `تويوتا.تغ` | Only `تغ` | `تويوتا.تغيير_لون_السيارة` |
| `اسم` | Current identifier prefix | Normal non-member replacement behavior |

The stored context is cleared after document mutation, focus loss, escape, an empty result, and successful insertion. If a member context is stale at activation time, the editor refuses the punctuation-unsafe word-boundary fallback rather than risk overwriting the receiver/dot.

## Verified `هذا` receiver

The semantic model already records a `هذا` parameter in a class method as an instance of the enclosing class. Member completion now resolves receiver names at the position *inside the receiver identifier*, not after the newly typed dot. This retains function/class scope when a current analysis snapshot is still pending.

A narrow provisional-snapshot rule permits only a verified `هذا` parameter with a valid `instanceClass`; arbitrary stale local variables remain excluded. Thus `هذا.` returns current class fields and methods, including a declared constructor method such as `تهيئة`, while preserving stale-result safety.

## Presentation rule

`PresentationClass::SelfReceiver` marks a parameter only when all conditions hold:

1. its symbol kind is `Parameter`;
2. its source spelling is exactly `هذا`;
3. it has a valid enclosing `instanceClass`.

The highlighter maps this class to the existing `TokenType::Self` theme format. A parameter merely named `هذا` outside a verified class instance context remains a generic parameter.

## Validation record

| Target | Result |
|---|---|
| Analysis integration tests | **11 passed, 0 failed**. Covers dot context ranges, class/object member completion, `هذا.` member completion, self receiver presentation, visual mapping, and scheduler regressions. |
| Full Qt application | Reconfigured and rebuilt successfully. |

## Remaining scope

Nested receivers (`a.b.c`), indexed receivers, aliases, and flow-sensitive reassignment require the future type/receiver analysis layer. Constructor syntax remains a language grammar decision; any valid method declared in a class scope is available through `هذا.` and instance completion.
