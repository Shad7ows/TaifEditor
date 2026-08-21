# TaifEditor Engineering Quality Gates

**Status:** Implemented and validated  
**Applies to:** Windows 11, Qt 6.11.1, MSVC 2022, qmake, all production targets, and all automated tests.

## Purpose

TaifEditor uses qmake and a Windows Qt/MSVC toolchain. This document turns the previously manual validation routine into a reproducible engineering gate without changing the editor’s Arabic/RTL user interface or requiring a build-system migration.

> **Release gate:** A deliverable is not considered validated until the production application builds, lexer/parser/semantic/analysis/UI suites pass, `git diff --check` passes, and generated validation artifacts are removed.

## Supported Entry Points

| Command | Intended use | Behavior |
|---|---|---|
| `scripts\validate_windows.cmd` | Local full validation | Initializes MSVC, validates Qt availability, builds the application and all supported tests, then runs whitespace hygiene. Existing local changes are allowed. |
| `scripts\validate_windows.cmd --ci` | Clean CI validation | Performs the same matrix but requires an empty `git status --porcelain` both before and after validation. This catches generated files that escape `.gitignore`. |
| `scripts\clean_validation_artifacts.cmd` | Local cleanup | Removes qmake test makefiles, test release/debug folders, stashes, and wrapper scripts. It does not alter source, documents, or the production application build tree. |

`QT_ROOT` and `VSDEVCMD` may be set before invoking the script to use an installed Qt kit or developer environment different from the default local values. The standard project configuration is Qt **6.11.1** with `msvc2022_64` and Visual Studio **2022** x64.

## Gate Matrix

| Gate | Target / command | Required evidence |
|---|---|---|
| Production integration | `taif/build/analysis_validation`, `qmake ..\..\Taif.pro`, `nmake /NOLOGO` | Full application links with production qmake source/MOC wiring. |
| Lexer | `tests/lexer/lexer_tests.pro` | Lexical grammar and tokenization suite passes. |
| Parser | `tests/parser/parser_tests.pro` | AST construction, recovery, and structural parsing suite passes. |
| Semantic | `tests/semantic/semantic_tests.pro` | Scope/symbol semantic suite passes. |
| Analysis | `tests/analysis/analysis_tests.pro` | Three-tier analysis controller suite passes. |
| Controllers | `tests/controllers/controllers_tests.pro` | Narrow output-buffer, asynchronous recovery-flush, and managed-run controller lifecycle suite passes. |
| UI/integration | `tests/ui/ui_tests.pro` | RTL surface, session/recovery, window lifecycle, document services, and bounded console behavior pass. |
| Patch hygiene | `git diff --check` | No whitespace errors. |
| Artifact hygiene | CI `git status --porcelain` | A clean checkout remains clean after the gate. |

## Test-Target Boundary

The test suite remains organized around stable project domains. Lexer, parser, semantic, analysis, and controllers targets are intentionally narrow and independent. `tests/controllers` validates output buffering, recovery flush behavior, and managed process lifecycle without pulling in editor widget construction. `tests/ui` remains the controlled integration target for main-window lifecycle, RTL surface behavior, session/persistence, document services, and console surfaces because those flows require real Qt object ownership and signal wiring.

New regression tests must be placed in the narrowest existing target that can exercise the behavior. A change to lexical, parser, or semantic logic must not be validated only through the UI target. A process, recovery, or top-level-window lifecycle change must add focused integration coverage even if the full application build compiles.

## Warning and Static-Analysis Policy

The production qmake projects require **C++17** and compile Arabic source literals using `/utf-8` on Windows. The supported baseline uses MSVC warning level W3; warnings are review signals and must not be hidden by broad suppression additions.

| Classification | Policy |
|---|---|
| New warning in changed code | Fix before merge, or document a specific narrowly scoped suppression with its reason. |
| Existing baseline warning | Track and remove when its owning code is touched. Current observed examples include C4100 in an unrelated `TEditor` constructor parameter and C4834 in pre-existing `SymbolTable` code. |
| Runtime Qt warning | Treat as a regression if new or caused by changed code. Existing test runtime output must be audited when changing the responsible component. |
| MSVC `/analyze` | Recommended as a non-blocking local/CI job for changed C++ modules. Promote only after the baseline is catalogued. |
| AddressSanitizer | Recommended for a debug-only Windows job. Do not combine it with the release delivery gate until Qt/MSVC toolchain compatibility and baseline results are recorded. |

## CI Contract

`.github/workflows/windows-qt-msvc.yml` runs on a Windows 2022 hosted runner, installs Qt 6.11.1 with the MSVC 2022 64-bit kit, and calls `scripts\validate_windows.cmd --ci`. Test/build logs are uploaded only on failure. CI must use a clean checkout; developers must not weaken the clean-tree check to accommodate generated files.

The workflow intentionally begins with one supported Windows matrix. Additional Qt versions, analyzers, and ASan jobs should be added as separate non-blocking jobs first, then promoted after their warnings and environment constraints are understood.

## Maintenance Rules

Do not add generated qmake files, release/debug test binaries, temporary validation batch files, or test logs to source control. Keep `scripts\clean_validation_artifacts.cmd` updated whenever a new test target produces a new artifact location. If a new test target is added, register it in `scripts\validate_windows.cmd`, this matrix, and CI before relying on it for acceptance.

The gate does not replace code review. In particular, maintain the established hardening invariants: no nested GUI event loops for service completion, no destructive persistence without acknowledgement/error handling, no worker callback that assumes a deleted widget is still alive, and no change that compromises Arabic/RTL behavior or the dark navy visual contract.
