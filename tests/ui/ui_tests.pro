QT += core gui widgets testlib
CONFIG += c++17 testcase
TEMPLATE = app
TARGET = TaifDockableToolsTests
win32:QMAKE_CXXFLAGS += /utf-8

INCLUDEPATH += \
    ../../source/console \
    ../../source/menubar \
    ../../source/components \
    ../../source/session \
    ../../source/texteditor/navigation \
    ../../source/language/semantic \
    ../../source/language/parser \
    ../../source/language/lexer

SOURCES += \
    tst_DockableTools.cpp \
    ../../source/console/TConsole.cpp \
    ../../source/console/DockableConsoleTool.cpp \
    ../../source/menubar/TMenu.cpp \
    ../../source/components/TSearchPanel.cpp \
    ../../source/components/SearchReplaceEngine.cpp \
    ../../source/session/SessionStore.cpp \
    ../../source/session/SessionEditorDialog.cpp \
    ../../source/texteditor/navigation/TBreadcrumbBar.cpp

HEADERS += \
    ../../source/console/TConsole.h \
    ../../source/console/DockableConsoleTool.h \
    ../../source/menubar/TMenu.h \
    ../../source/components/TSearchPanel.h \
    ../../source/components/SearchReplaceEngine.h \
    ../../source/session/SessionStore.h \
    ../../source/session/SessionEditorDialog.h \
    ../../source/texteditor/navigation/BreadcrumbTypes.h \
    ../../source/texteditor/navigation/TBreadcrumbBar.h
