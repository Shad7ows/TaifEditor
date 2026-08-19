QT += core testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = TaifLexerTests

# win32:QMAKE_CXXFLAGS += /utf-8

INCLUDEPATH += ../../source/language/lexer

SOURCES += \
    tst_TaifLexer.cpp \
    ../../source/language/lexer/TaifLexer.cpp

HEADERS += \
    ../../source/language/lexer/TaifLexer.h
