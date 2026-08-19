QT += core gui widgets testlib
CONFIG += c++17 testcase
TEMPLATE = app
TARGET = TaifDockableToolsTests
win32:QMAKE_CXXFLAGS += /utf-8

INCLUDEPATH += \
    ../../source/console \
    ../../source/menubar

SOURCES += \
    tst_DockableTools.cpp \
    ../../source/console/TConsole.cpp \
    ../../source/console/DockableConsoleTool.cpp \
    ../../source/menubar/TMenu.cpp

HEADERS += \
    ../../source/console/TConsole.h \
    ../../source/console/DockableConsoleTool.h \
    ../../source/menubar/TMenu.h
