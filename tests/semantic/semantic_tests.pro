QT += core testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = TaifSemanticTests

# win32:QMAKE_CXXFLAGS += /utf-8

INCLUDEPATH += \
    ../../source/language/lexer \
    ../../source/language/parser \
    ../../source/language/semantic

SOURCES += \
    tst_SymbolTable.cpp \
    ../../source/language/lexer/TaifLexer.cpp \
    ../../source/language/parser/TaifParser.cpp \
    ../../source/language/semantic/SymbolTable.cpp

HEADERS += \
    ../../source/language/lexer/TaifLexer.h \
    ../../source/language/parser/TaifParser.h \
    ../../source/language/semantic/SymbolTable.h
