QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++23

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


TARGET = Taif

RESOURCES += \
    resources.qrc


# Include directories
INCLUDEPATH +=  ../source/texteditor \
                ../source/menubar   \
                ../source/settings  \
                ../source/components    \

SOURCES += \
    ../source/components/TFlatButton.cpp \
    Taif.cpp \
    main.cpp     \
    ../source/texteditor/AlifComplete.cpp \
    ../source/texteditor/AlifLexer.cpp \
    ../source/texteditor/TEditor.cpp \
    ../source/texteditor/THighlighter.cpp \
    ../source/menubar/TMenu.cpp    \
    ../source/settings/TSettings.cpp   \

HEADERS += \
    ../source/components/TFlatButton.h \
    Taif.h  \
    ../source/texteditor/AlifComplete.h \
    ../source/texteditor/AlifLexer.h \
    ../source/texteditor/TEditor.h \
    ../source/texteditor/THighlighter.h \
    ../source/menubar/TMenu.h  \
    ../source/settings/TSettings.h \



# Add the application icon (Windows)
win32:RC_ICONS += resources/TaifLogo.ico

# Add the application icon (macOS/Linux)
macx:ICON = resources/TaifLogo.icns
unix:!macx:ICON = resources/TaifLogo.png

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target




