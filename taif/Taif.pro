QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Source files contain Arabic language literals and grammar keywords.
win32:QMAKE_CXXFLAGS += /utf-8

TARGET = Taif

RESOURCES += resources.qrc

INCLUDEPATH += \
    ../source/texteditor \
    ../source/texteditor/highlighter \
    ../source/texteditor/autocomplete \
    ../source/texteditor/hover \
    ../source/texteditor/diagnostics \
    ../source/language/lexer \
    ../source/language/parser \
    ../source/language/semantic \
    ../source/language/presentation \
    ../source/texteditor/analysis \
    ../source/components \
    ../source/console \
    ../source/menubar \
    ../source/pages \
    ../source/settings

SOURCES += \
    ../source/texteditor/autocomplete/AutoComplete.cpp \
    ../source/texteditor/autocomplete/AutoCompleteUI.cpp \
    ../source/texteditor/highlighter/TLexer.cpp \
    ../source/language/lexer/TaifLexer.cpp \
    ../source/language/parser/TaifParser.cpp \
    ../source/language/semantic/SymbolTable.cpp \
    ../source/language/presentation/SemanticPresentationAdapter.cpp \
    ../source/language/presentation/DiagnosticPresentationAdapter.cpp \
    ../source/texteditor/analysis/EditorAnalysisController.cpp \
    ../source/texteditor/analysis/SemanticCompletionProvider.cpp \
    ../source/texteditor/analysis/SemanticHoverProvider.cpp \
    ../source/texteditor/analysis/SemanticDefinitionProvider.cpp \
    ../source/texteditor/hover/HoverPopup.cpp \
    ../source/texteditor/diagnostics/DiagnosticsPanel.cpp \
    ../source/texteditor/highlighter/TSyntaxDefinition.cpp \
    ../source/texteditor/highlighter/TSyntaxHighlighter.cpp \
    Taif.cpp \
    main.cpp \
    ../source/texteditor/TEditor.cpp \
    ../source/components/TMinimap.cpp \
    ../source/components/TFlatButton.cpp \
    ../source/components/TSearchPanel.cpp \
    ../source/components/SearchReplaceEngine.cpp \
    ../source/console/TConsole.cpp \
    ../source/console/DockableConsoleTool.cpp \
    ../source/console/ProcessWorker.cpp \
    ../source/menubar/TMenu.cpp \
    ../source/pages/TWelcomeWindow.cpp \
    ../source/settings/TSettings.cpp

HEADERS += \
    ../source/texteditor/autocomplete/AutoComplete.h \
    ../source/texteditor/autocomplete/AutoCompleteUI.h \
    ../source/texteditor/highlighter/TLexer.h \
    ../source/language/lexer/TaifLexer.h \
    ../source/language/parser/TaifParser.h \
    ../source/language/semantic/SymbolTable.h \
    ../source/language/presentation/LanguageAnalysis.h \
    ../source/language/presentation/SemanticPresentationAdapter.h \
    ../source/language/presentation/DiagnosticPresentationAdapter.h \
    ../source/texteditor/analysis/EditorAnalysisController.h \
    ../source/texteditor/analysis/SemanticCompletionProvider.h \
    ../source/texteditor/analysis/SemanticHoverProvider.h \
    ../source/texteditor/analysis/SemanticDefinitionProvider.h \
    ../source/texteditor/hover/HoverPopup.h \
    ../source/texteditor/diagnostics/DiagnosticsPanel.h \
    ../source/texteditor/highlighter/TSyntaxDefinition.h \
    ../source/texteditor/highlighter/TSyntaxHighlighter.h \
    ../source/texteditor/highlighter/TSyntaxThemes.h \
    ../source/texteditor/highlighter/TToken.h \
    Taif.h \
    ../source/texteditor/TEditor.h \
    ../source/components/TMinimap.h \
    ../source/components/TFlatButton.h \
    ../source/components/TSearchPanel.h \
    ../source/components/SearchReplaceEngine.h \
    ../source/console/TConsole.h \
    ../source/console/DockableConsoleTool.h \
    ../source/console/ProcessWorker.h \
    ../source/menubar/TMenu.h \
    ../source/pages/TWelcomeWindow.h \
    ../source/settings/TSettings.h

# Add the application icon (Windows)
win32:RC_ICONS += resources/TaifLogo.ico

# Add the application icon (macOS/Linux)
macx:ICON = resources/TaifLogo.icns
unix:!macx:ICON = resources/TaifLogo.png

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else:unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
