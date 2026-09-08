QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++23

TARGET = Taif

RESOURCES += \
    resources.qrc


# Include directories
INCLUDEPATH +=  ../source/texteditor \
                ../source/texteditor/highlighter \
                ../source/texteditor/autocomplete \
                ../source/components \
                ../source/console \
                ../source/menubar   \
                ../source/pages \
                ../source/settings  \
                ../source/aliflang/lexer  \
                ../source/aliflang/parser  \
                ../source/aliflang/semantic  \

SOURCES += \
    Taif.cpp \
    main.cpp \
    ../source/texteditor/autocomplete/AutoComplete.cpp \
    ../source/texteditor/autocomplete/AutoCompleteUI.cpp \
    ../source/texteditor/highlighter/TLexer.cpp \
    ../source/texteditor/highlighter/TSyntaxDefinition.cpp \
    ../source/texteditor/highlighter/TSyntaxHighlighter.cpp \
    ../source/texteditor/TEditor.cpp \
    ../source/components/TMinimap.cpp \
    ../source/components/TSearchPanel.cpp \
    ../source/console/TConsole.cpp \
    ../source/console/ProcessWorker.cpp \
    ../source/menubar/TMenu.cpp    \
    ../source/pages/TWelcomeWindow.cpp  \
    ../source/settings/TSettings.cpp   \
    ../source/aliflang/lexer/AlifLexer.cpp \
    ../source/aliflang/parser/AlifParser.cpp \
    ../source/aliflang/semantic/AlifSymbolTable.cpp \

HEADERS += \
    Taif.h  \
    ../source/texteditor/autocomplete/AutoComplete.h \
    ../source/texteditor/autocomplete/AutoCompleteUI.h \
    ../source/texteditor/highlighter/TLexer.h \
    ../source/texteditor/highlighter/TSyntaxDefinition.h \
    ../source/texteditor/highlighter/TSyntaxHighlighter.h \
    ../source/texteditor/highlighter/TSyntaxThemes.h \
    ../source/texteditor/highlighter/TToken.h \
    ../source/texteditor/TEditor.h \
    ../source/components/TMinimap.h \
    ../source/components/TSearchPanel.h \
    ../source/console/TConsole.h \
    ../source/console/ProcessWorker.h \
    ../source/menubar/TMenu.h  \
    ../source/pages/TWelcomeWindow.h \
    ../source/settings/TSettings.h \
    ../source/aliflang/lexer/AlifLexer.h \
    ../source/aliflang/parser/AlifParser.h \
    ../source/aliflang/semantic/AlifSymbolTable.h \



# Add the application icon (Windows)
win32:RC_ICONS += resources/TaifLogo.ico

# Add the application icon (macOS/Linux)
macx:ICON = resources/TaifLogo.icns
unix:!macx:ICON = resources/TaifLogo.png

# Default rules for deployment.
qnx:target.path = /tmp/$${TARGET}/bin
else:unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


