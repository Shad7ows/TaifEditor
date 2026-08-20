# TaifEditor Breadcrumb Navigation

**Status:** Implemented and validated  
**Applies to:** `SemanticModel`, `TEditor`, `EditorBreadcrumbContext`, `TBreadcrumbBar`, and `Taif`

## Purpose

The breadcrumb bar provides a compact, clickable description of the user’s current editing location. It appears above the document tabs and presents the active file’s immediate parent folder and filename followed by the enclosing semantic declaration path at the cursor. A cursor inside a method of a class, for example, can be represented as `المشروع ‹ سيارة.alif ‹ صنف سيارة ‹ دالة تغيير_اللون`.

The feature is deliberately split between semantic query, editor context, presentation, and main-window navigation. This keeps the widget independent of parsing and document state, prevents stale analysis from being shown as current, and allows each layer to be tested without constructing the complete application.

## Architecture and Data Ownership

| Layer | Primary responsibility | Owned data | Must not do |
|---|---|---|---|
| `SemanticModel` | Resolve the scope path enclosing a UTF-16 offset. | Immutable symbols, scopes, and `m_scopeOwnerSymbols`. | Read editor widgets or decide UI styling. |
| `TEditor` | Convert the active cursor position into a revision-safe `EditorBreadcrumbContext`. | Current analysis snapshot and cursor offset. | Build controls or reveal paths in the file tree. |
| `BreadcrumbTypes.h` | Define the lightweight boundary contract. | `EditorBreadcrumbContext`. | Depend on `TEditor` or main-window implementation. |
| `TBreadcrumbBar` | Render RTL file and symbol controls, then emit navigation requests. | File context and the last safe semantic context. | Run analysis, read `QTextDocument`, or mutate tabs. |
| `Taif` | Coordinate the active editor, folder tree, tabs, and navigation actions. | Active editor binding and loaded-folder context. | Reimplement semantic scope lookup. |

> **Boundary rule:** `TBreadcrumbBar` is presentation-only. It receives already validated context and emits intents; it does not know how language analysis is performed.

## Semantic Path Construction

`SemanticModel::enclosingSymbolPathAt()` walks the most specific containing scope upward to the module boundary. Each scope is mapped to its owning declaration through the O(1) `m_scopeOwnerSymbols` index, populated when classes and functions are indexed. The resulting declarations are returned outermost first, which produces an intuitive visual order: class before method.

| Query condition | Result |
|---|---|
| Cursor is inside a class body | The class declaration is included. |
| Cursor is inside a method within a class | The class followed by the method is included. |
| Cursor is in module-level code or a non-declaration scope | The semantic suffix is empty. |
| A scope has no indexed declaration owner | The scope is skipped safely. |
| A range does not contain the requested UTF-16 offset | It is never selected as enclosing context. |

All offsets remain UTF-16 offsets. This matches `QString`, `QTextDocument`, lexical source ranges, parser ranges, diagnostics, and editor navigation, and it is especially important for Arabic identifiers.

## Revision-Safety Contract

Semantic breadcrumbs are valid only when the analysis snapshot revision equals `EditorAnalysisController::currentRevision()`. `TEditor::breadcrumbContextAtCursor()` applies that gate before consulting the semantic model. On a document mutation, the editor clears its applied semantic snapshot and emits an empty breadcrumb context immediately; only the later analysis result for the current revision may repopulate the semantic suffix.

| Event | Required breadcrumb behavior |
|---|---|
| Cursor movement | Recompute and emit the context for the current cursor offset. |
| Text mutation | Remove semantic segments immediately, while retaining the file trail. |
| Current-revision semantic analysis applied | Recompute the active cursor context and emit it. |
| Stale analysis result | Never expose its symbols in the breadcrumb bar. |
| Tab activation | Disconnect the prior editor context callback, bind exactly one callback for the new editor, then refresh both file and semantic context. |

This gate is not optional. Removing it would allow declaration paths from an older document revision to appear after the source has changed.

## File Trail and Navigation Behavior

`Taif::refreshBreadcrumbs()` supplies only the active editor’s file path to the bar. For an unsaved editor, the bar displays `بدون عنوان`. For every saved file, the visible trail contains exactly two file controls: its immediate parent folder and its filename. Ancestor directories, workspace roots, drive letters, and absolute paths remain available only in tooltips and are never added as visible segments.

A file-segment click emits `fileSegmentActivated(path)`. `Taif::revealBreadcrumbPath()` expands the corresponding location in the file tree only when the path is within the loaded folder root; it does not open files, change buffer contents, or navigate outside the workspace tree. A semantic-segment click emits the declaration `SourceRange`, which the main window routes through `TEditor::navigateToDiagnosticRange()`.

> **Navigation rule:** File controls reveal existing tree locations; symbol controls move the editor cursor to an existing declaration. Neither action performs semantic analysis or changes source text.

## RTL Presentation and Accessibility

The bar, its layout, and all segment controls are right-to-left. Its parent RTL direction mirrors the layout’s logical insertion order, placing the first visible segment at the right edge and each later segment to its left. Arabic declaration labels precede their names: `صنف` for classes and `دالة` for functions. The navy/slate visual surface matches the editor’s existing completion and navigation surfaces, while semantic segments use a distinct blue to indicate an editor-navigation target.

| Element | Stable identifier or accessible description | Purpose |
|---|---|---|
| Bar | `BreadcrumbBar` | Locates the navigation surface. |
| File segment | `BreadcrumbFileSegment<N>` | Identifies each ordered file-path control. |
| Semantic segment | `BreadcrumbSemanticSegment` with `breadcrumbIndex` property | Identifies a declaration-navigation control. |
| Separator | `BreadcrumbSeparator` | Provides a non-interactive path boundary. |
| Untitled file | `ملف: بدون عنوان` | Preserves usable context before the first save. |

Tooltips preserve complete paths for file segments and visible Arabic symbol labels for semantic segments. Segment reconstruction deletes superseded controls synchronously, so no inaccessible or stale controls remain between context changes.

## Lifecycle Integration

The main window refreshes the breadcrumb state after opening a file, saving or saving-as, loading a folder, restoring a saved session, and changing the active tab. The new-file path relies on the active-tab change signal, which binds and refreshes the newly selected untitled editor. The active editor signal connection is stored in `breadcrumbConnection` and is disconnected before the next binding, preventing duplicate cursor-context callbacks as users switch tabs.

`loadFolder()` owns the `folderPath` member update for file-tree reveal safety. The breadcrumb bar no longer uses that root to construct its visible file trail; callers must still avoid setting it opportunistically, because a failed or partial tree transition could otherwise make tree reveal behavior disagree about the active root.

## Regression Coverage and Required Validation

| Target | Coverage relevant to breadcrumbs |
|---|---|
| `tests/semantic` | Nested class/function scope paths are returned in outer-to-inner order. |
| `tests/ui` | RTL direction and geometry, untitled rendering, immediate-parent file trail, ordered semantic segments, semantic clearing, file click payloads, and symbol-range click payloads. |
| Full `taif` application build | Confirms production registration, MOC generation, and main-window/editor integration. |
| Lexer, parser, analysis, and prior UI tests | Guard the language pipeline and existing editor surfaces that provide breadcrumb inputs. |

Before accepting breadcrumb changes, run the full application build and all lexer, parser, semantic, analysis, and UI suites on the supported Qt/MSVC toolchain. Do not treat a widget-only test run as sufficient validation of main-window integration.

## Maintenance Rules

Keep `EditorBreadcrumbContext` in `BreadcrumbTypes.h`; moving it back into `TEditor.h` reintroduces an unnecessary dependency from a presentation widget to the full editor implementation. Preserve the revision comparison before symbol queries, the outer-to-inner ordering of `SemanticBreadcrumb` values, and the UTF-16 coordinate convention. New semantic scope kinds may participate in breadcrumbs only after they have a clear declaration-owner mapping and a deliberate Arabic presentation label.

Do not use delayed deletion when rebuilding breadcrumb controls. The bar represents immediate state, and deferred controls can leave old paths visible or discoverable until the next event-loop turn. Preserve the two-control saved-file trail—immediate parent folder followed by filename—and do not reintroduce workspace roots, ancestor directories, drive letters, or absolute paths as visible controls. Keep the frame and every segment right-to-left, while leaving the layout’s logical direction left-to-right so parent RTL mirroring places the first segment on the right. Do not make file-segment activation open source files automatically, and do not allow the reveal helper to select paths outside the loaded root. Finally, when adding a new refresh trigger, verify both the file trail and semantic suffix: file state is lifecycle-driven, while symbol state is cursor- and revision-driven.
