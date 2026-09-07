QT += core testlib
CONFIG += c++17 testcase console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = TLexerTests

INCLUDEPATH += ../../source/aliflang/lexer

SOURCES += \
    TLexerTest.cpp \
    ../../source/aliflang/lexer/TLexer.cpp

HEADERS += \
    ../../source/aliflang/lexer/TLexer.h
