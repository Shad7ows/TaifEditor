QT += core widgets testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = TaifAnalysisTests

# win32:QMAKE_CXXFLAGS += /utf-8

INCLUDEPATH += \
    ../../source/language/lexer \
    ../../source/language/parser \
    ../../source/language/semantic \
    ../../source/language/presentation \
    ../../source/texteditor/analysis \
    ../../source/texteditor/autocomplete

SOURCES += \
    tst_EditorAnalysisController.cpp \
    ../../source/language/lexer/TaifLexer.cpp \
    ../../source/language/parser/TaifParser.cpp \
    ../../source/language/semantic/SymbolTable.cpp \
    ../../source/language/presentation/SemanticPresentationAdapter.cpp \
    ../../source/texteditor/analysis/EditorAnalysisController.cpp \
    ../../source/texteditor/analysis/SemanticCompletionProvider.cpp \
    ../../source/texteditor/autocomplete/AutoCompleteUI.cpp

HEADERS += \
    ../../source/language/lexer/TaifLexer.h \
    ../../source/language/parser/TaifParser.h \
    ../../source/language/semantic/SymbolTable.h \
    ../../source/language/presentation/LanguageAnalysis.h \
    ../../source/language/presentation/SemanticPresentationAdapter.h \
    ../../source/texteditor/analysis/EditorAnalysisController.h \
    ../../source/texteditor/analysis/SemanticCompletionProvider.h \
    ../../source/texteditor/analysis/CompletionContext.h \
    ../../source/texteditor/autocomplete/AutoComplete.h \
    ../../source/texteditor/autocomplete/AutoCompleteUI.h
