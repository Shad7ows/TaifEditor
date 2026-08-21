QT += core gui widgets testlib
CONFIG += c++17 testcase
TEMPLATE = app
TARGET = TaifDockableToolsTests
win32:QMAKE_CXXFLAGS += /utf-8

RESOURCES += ../../taif/resources.qrc

INCLUDEPATH += \
    ../../source/console \
    ../../source/menubar \
    ../../source/components \
    ../../source/pages \
    ../../source/session \
    ../../source/texteditor \
    ../../source/texteditor/autocomplete \
    ../../source/texteditor/analysis \
    ../../source/texteditor/hover \
    ../../source/texteditor/diagnostics \
    ../../source/texteditor/navigation \
    ../../source/language/presentation \
    ../../source/language/semantic \
    ../../source/language/parser \
    ../../source/language/lexer \
    ../../source/settings \
    ../../source/recovery \
    ../../source/run \
    ../../source/texteditor/highlighter \
    ../../taif

SOURCES += \
    tst_DockableTools.cpp \
    ../../source/console/TConsole.cpp \
    ../../source/console/DockableConsoleTool.cpp \
    ../../source/menubar/TMenu.cpp \
    ../../source/components/TSearchPanel.cpp \
    ../../source/components/SearchReplaceEngine.cpp \
    ../../source/components/TFlatButton.cpp \
    ../../source/session/SessionStore.cpp \
    ../../source/session/SessionEditorDialog.cpp \
    ../../source/settings/EditorPreferences.cpp \
    ../../source/settings/TSettings.cpp \
    ../../source/recovery/RecoveryStore.cpp \
    ../../source/recovery/RecoveryCoordinator.cpp \
    ../../source/recovery/TRecoveryDialog.cpp \
    ../../source/texteditor/autocomplete/AutoComplete.cpp \
    ../../source/texteditor/autocomplete/AutoCompleteUI.cpp \
    ../../source/texteditor/highlighter/TLexer.cpp \
    ../../source/language/lexer/TaifLexer.cpp \
    ../../source/language/parser/TaifParser.cpp \
    ../../source/language/semantic/SymbolTable.cpp \
    ../../source/language/presentation/SemanticPresentationAdapter.cpp \
    ../../source/language/presentation/DiagnosticPresentationAdapter.cpp \
    ../../source/texteditor/analysis/EditorAnalysisController.cpp \
    ../../source/texteditor/analysis/SemanticCompletionProvider.cpp \
    ../../source/texteditor/analysis/SemanticHoverProvider.cpp \
    ../../source/texteditor/analysis/SemanticDefinitionProvider.cpp \
    ../../source/texteditor/hover/HoverPopup.cpp \
    ../../source/texteditor/diagnostics/DiagnosticsPanel.cpp \
    ../../source/texteditor/highlighter/TSyntaxDefinition.cpp \
    ../../source/texteditor/highlighter/TSyntaxHighlighter.cpp \
    ../../source/texteditor/TEditor.cpp \
    ../../source/components/TMinimap.cpp \
    ../../source/run/AlifRunController.cpp \
    ../../source/texteditor/navigation/TBreadcrumbBar.cpp \
    ../../source/pages/TWelcomeWindow.cpp \
    ../../taif/ApplicationBootstrap.cpp \
    ../../taif/ApplicationWindowController.cpp \
    ../../taif/Taif.cpp

HEADERS += \
    ../../source/console/TConsole.h \
    ../../source/console/DockableConsoleTool.h \
    ../../source/menubar/TMenu.h \
    ../../source/components/TSearchPanel.h \
    ../../source/components/SearchReplaceEngine.h \
    ../../source/session/SessionStore.h \
    ../../source/session/SessionEditorDialog.h \
    ../../source/settings/EditorPreferences.h \
    ../../source/settings/TSettings.h \
    ../../source/recovery/RecoveryStore.h \
    ../../source/recovery/RecoveryCoordinator.h \
    ../../source/recovery/TRecoveryDialog.h \
    ../../source/texteditor/TEditor.h \
    ../../source/texteditor/autocomplete/AutoComplete.h \
    ../../source/texteditor/autocomplete/AutoCompleteUI.h \
    ../../source/texteditor/analysis/EditorAnalysisController.h \
    ../../source/texteditor/analysis/SemanticCompletionProvider.h \
    ../../source/texteditor/analysis/SemanticHoverProvider.h \
    ../../source/texteditor/analysis/SemanticDefinitionProvider.h \
    ../../source/texteditor/hover/HoverPopup.h \
    ../../source/texteditor/diagnostics/DiagnosticsPanel.h \
    ../../source/texteditor/highlighter/TSyntaxDefinition.h \
    ../../source/texteditor/highlighter/TSyntaxHighlighter.h \
    ../../source/components/TMinimap.h \
    ../../source/components/TFlatButton.h \
    ../../source/pages/TWelcomeWindow.h \
    ../../source/run/AlifRunController.h \
    ../../source/texteditor/navigation/BreadcrumbTypes.h \
    ../../source/texteditor/navigation/TBreadcrumbBar.h \
    ../../taif/ApplicationBootstrap.h \
    ../../taif/ApplicationWindowController.h \
    ../../taif/Taif.h
