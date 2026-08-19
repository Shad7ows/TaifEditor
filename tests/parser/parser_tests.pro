QT += core testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = TaifParserTests

# win32:QMAKE_CXXFLAGS += /utf-8

INCLUDEPATH += \
    ../../source/language/lexer \
    ../../source/language/parser

SOURCES += \
    tst_TaifParser.cpp \
    ../../source/language/lexer/TaifLexer.cpp \
    ../../source/language/parser/TaifParser.cpp

HEADERS += \
    ../../source/language/lexer/TaifLexer.h \
    ../../source/language/parser/TaifParser.h
