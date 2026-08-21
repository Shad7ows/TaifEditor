# TaifEditor Settings Architecture

**Status:** Implemented and validated  
**Applies to:** `EditorPreferences`, `PreferencesStore`, `TSettings`, `TEditor`, `TSyntaxHighlighter`, `Taif`, and recent-file persistence

## Purpose

TaifEditor settings use a typed preference model so that the Arabic/RTL settings interface remains a presentation layer rather than becoming the owner of editor behavior. The existing dark navy design is deliberately retained. New settings pages reuse the established sidebar, surface colors, typography, group-box style, input style, and primary-action color.

The settings UI changes a draft snapshot first. It previews valid values in open editors, persists only through **تطبيق**, restores the baseline through **إلغاء** or the window close action, and exposes **استعادة الافتراضيات** without silently saving it.

## Ownership Model

| Component | Ownership and responsibility | Must not do |
|---|---|---|
| `EditorPreferences` | Holds typed editor and workspace values with safe defaults. | Read widgets or access `QSettings` directly. |
| `PreferencesStore` | Loads, normalizes, reads the existing schema keys, and persists a complete preference snapshot with an explicit success/error result. | Update editor widgets or display dialogs. |
| `TSettings` | Presents Arabic/RTL controls, maintains baseline and draft snapshots, emits preview/apply signals, and surfaces persistence failures. | Write raw settings keys from controls or directly manipulate editors. |
| `Taif` | Connects settings once and forwards complete preference snapshots to all live editors. | Create duplicate per-control connections whenever the settings window opens. |
| `TEditor` | Applies a complete normalized snapshot to typography, wrapping, editor aids, autosave, hover, completion, and inline diagnostics. | Persist user preferences. |
| `TSyntaxHighlighter` | Shows or hides diagnostic wave underlines while retaining lexical and semantic colors. | Change semantic diagnostic data or revision safety. |

> **Invariant:** A new setting must be represented in `EditorPreferences`, normalized by `PreferencesStore`, rendered by `TSettings`, applied by `TEditor` or its owning surface, and covered by a regression before it is considered complete.

## Preference Contract

| Preference | Default | Live behavior |
|---|---:|---|
| Font family and size | Noto Kufi Arabic, 18 px | Updates existing and new editors. Font choices come from the named application font catalog. |
| Syntax theme | First supported theme | Reapplies the existing syntax theme to every open editor. |
| Tab width | 8 spaces | Recalculates `QPlainTextEdit` tab-stop distance. |
| Word wrap | Enabled | Switches wrapping and horizontal-scrollbar policy. |
| Line numbers | Visible | Hides/shows the line-number area and recalculates viewport margins. |
| Minimap | Visible | Hides/shows the existing minimap and recalculates viewport margins. |
| Current-line highlight | Enabled | Keeps Ctrl+hover definition links while optionally suppressing the line background. |
| Autosave | Enabled, 60 s | Enables/disables the existing recovery-backup timer and clamps the interval from 5–300 seconds. |
| Automatic completion | Enabled | Suppresses automatic popups when disabled but preserves explicit `Ctrl+Space` completion. |
| Hover information | Enabled, 350 ms | Enables/disables semantic hover; the delay is bounded from 100–1500 ms. |
| Inline diagnostics | Visible | Shows/hides error and warning wave underlines without changing semantic diagnostics. |
| Recent files | 10 entries | Bounds recent-file retention from 0–30. A limit of zero disables history. |

## Draft Workflow

The settings window follows this sequence.

| User action | Draft state | Open-editor state | Persistent state |
|---|---|---|---|
| Open settings | Reloads current persisted snapshot as baseline and draft. | Unchanged. | Unchanged. |
| Change a control | Updates and normalizes the draft. | Previewed immediately across current editors. | Unchanged. |
| Click `مسح سجل الملفات الأخيرة` | Records a pending clear action and disables the button with an Arabic pending label. | Unchanged. | Unchanged. |
| Click `تطبيق` | Draft becomes new baseline only after a successful store result. | Already previewed. | Preferences and any pending recent-file clear are committed together. |
| Failed `تطبيق` | Draft and preview remain active. | Unchanged. | Unchanged; an Arabic error is displayed. |
| Click `إلغاء` | Draft and any pending clear action are discarded; baseline is restored. | Baseline reapplied. | Unchanged. |
| Close the settings window | Equivalent to cancel. | Baseline reapplied. | Unchanged. |
| Click `استعادة الافتراضيات` | Draft becomes defaults. | Defaults previewed. | Unchanged until Apply. |

## Arabic/RTL Design Rules

The settings window remains RTL. Arabic text is first in labels, descriptions, and actions. Numeric fields such as tab width, seconds, and milliseconds retain natural numeric interaction. Do not introduce a new application color system, different sidebar pattern, or light-mode visual redesign through ordinary settings additions.

Stable object names are part of the regression surface. New settings controls use `Settings...` names so focused tests can assert interaction behavior without depending on text rendering or layout coordinates.

## Safety Rules

| Area | Required rule |
|---|---|
| Semantic analysis | Preference changes must never bypass revision checks, alter the 0/150/300 ms default pipeline, or reopen a dismissed completion popup. |
| Completion | Disabling automatic completion may hide automatic popups only; it must preserve explicit user-invoked completion. |
| Hover | Disabling hover must dismiss a visible tooltip and stop future scheduling; it must not change symbol resolution. |
| Diagnostics | Hiding inline underlines is presentation-only; the Problems panel and diagnostic model retain their current data. |
| File safety | Autosave controls govern recovery backups only. They do not weaken atomic source-file saves through `QSaveFile`. |
| Recent files | A zero limit clears/avoids history but must not affect explicitly opened files, saved sessions, or command-line launch behavior. The explicit clear button is draft-local and cannot remove history until Apply succeeds. |
| Font catalog | No settings consumer may use positional `QFontDatabase::applicationFontFamilies(index)` lookups. |

## Extension Procedure

When adding another preference, first add a field and default to `EditorPreferences`, then enforce its bounds in `PreferencesStore::normalize()`. Add the store key in `load()` and `save()`, give it an Arabic/RTL control in `TSettings`, include it in the draft synchronization methods, and apply it through the relevant component. Update this document and add a focused test before full validation.

Settings that need a missing backend, such as formatter-on-save, external-file watching, command shortcut editing, or execution-controller policy, must not be displayed as operational until that backend and its regression coverage exist.

## Regression Requirements

The focused UI target covers preference normalization and the settings window’s RTL draft workflow, including preview, cancel restoration, reset behavior, stable action controls, and draft-local recent-file clearing. The full application build validates `TSettings`, preference propagation, minimap font catalog integration, highlighter diagnostics visibility, and main-window recent-file behavior.

Before delivery, run the full application build plus lexer, parser, semantic, analysis, and UI suites. Remove generated qmake files, release/debug outputs, temporary scripts, and build logs. Finally run `git diff --check`.
