QT += core testlib
CONFIG += c++17 testcase
TEMPLATE = app
TARGET = TaifControllerTests
win32:QMAKE_CXXFLAGS += /utf-8

INCLUDEPATH += \
    ../../source/console \
    ../../source/recovery \
    ../../source/run

SOURCES += \
    tst_Controllers.cpp \
    ../../source/console/OutputBuffer.cpp \
    ../../source/recovery/RecoveryStore.cpp \
    ../../source/recovery/RecoveryCoordinator.cpp \
    ../../source/run/AlifRunController.cpp

HEADERS += \
    ../../source/console/OutputBuffer.h \
    ../../source/recovery/RecoveryStore.h \
    ../../source/recovery/RecoveryCoordinator.h \
    ../../source/run/AlifRunController.h
