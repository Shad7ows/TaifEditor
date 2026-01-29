#include "TMenu.h"

TMenuBar::TMenuBar(QWidget* parent) {

    parent->setStyleSheet(R"(
        QMenuBar {
            color: #dddddd;
            background-color: #1e202e;
            spacing: 5px; /* spacing between menu bar items */
        }

        QMenuBar::item {
            padding: 3px 7px;
            background: transparent;
            border-radius: 1px;
        }

        QMenuBar::item:selected { /* when selected using mouse or keyboard */
            background: #303349;
        }

        QMenuBar::item:pressed {
            background: #373a54;
        }
    )");

    QMenu* fileMenu = addMenu("ملف");
    //QMenu* editMenu = addMenu("تحرير");
    QMenu* runMenu = addMenu("تشغيل");
    QMenu* helpMenu = addMenu("مساعدة");

    fileMenu->setMinimumWidth(200);
    //editMenu->setMinimumWidth(200);
    runMenu->setMinimumWidth(200);
    helpMenu->setMinimumWidth(200);

    //QAction* folderAction = new QAction("فتح مجلد", parent);
    QAction* newAction = new QAction("جديد", parent);
    QAction* openFileAction = new QAction("فتح ملف", parent);
    QAction* openFolderAction = new QAction("فتح مجلد", parent);
    QAction* saveAction = new QAction("حفظ", parent);
    QAction* saveAsAction = new QAction("حفظ باسم", parent);
    QAction* SettingsAction = new QAction("الإعدادات", parent);
    QAction* exitAction = new QAction("خروج", parent);

    QAction* runAction = new QAction("تشغيل", parent);

    QAction* aboutAction = new QAction("عن المحرر", parent);
    QAction* updateAction = new QAction("تحديث النسخة", parent);


    //fileMenu->addAction(folderAction);
    //fileMenu->addSeparator();
    fileMenu->addAction(newAction);
    fileMenu->addAction(openFileAction);
    fileMenu->addAction(openFolderAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(SettingsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    runMenu->addAction(runAction);

    helpMenu->addAction(aboutAction);
    helpMenu->addAction(updateAction);


    QString style = R"(
        QMenu {
            color: #dddddd;
            background-color: #1e202e;
            border-bottom: 1px solid #10a8f4;
            border-left: 1px solid #10a8f4;
            border-radius: 3px;
        }
        QMenu::item {
            border: 5px solid transparent;
        }
        QMenu::item:selected {
            margin-left: 3px;
        }
        QMenu::separator {
            height: 1px;
            background: #303349;
            margin-left: 15px;
            margin-right: 10px;
        }
        QMenu::indicator {
            width: 1px;
            height: 1px;
        }
)";
    fileMenu->setStyleSheet(style);
    //editMenu->setStyleSheet(style);
    runMenu->setStyleSheet(style);
    helpMenu->setStyleSheet(style);


    connect(newAction, &QAction::triggered, this, &TMenuBar::onNewAction);
    connect(openFileAction, &QAction::triggered, this, &TMenuBar::onOpenFileAction);
    connect(openFolderAction, &QAction::triggered, this, &TMenuBar::onOpenFolderAction);
    connect(saveAction, &QAction::triggered, this, &TMenuBar::onSaveAction);
    connect(saveAsAction, &QAction::triggered, this, &TMenuBar::onSaveAsAction);
    connect(SettingsAction, &QAction::triggered, this, &TMenuBar::onSettingsAction);
    connect(exitAction, &QAction::triggered, this, &TMenuBar::onExitApp);

    connect(runAction, &QAction::triggered, this, &TMenuBar::onRunAction);

    connect(aboutAction, &QAction::triggered, this, &TMenuBar::onAboutAction);
    connect(updateAction, &QAction::triggered, this, &TMenuBar::onUpdateAction);
}

void TMenuBar::onNewAction() { emit newRequested(); }
void TMenuBar::onOpenFileAction() { emit openFileRequested(); }
void TMenuBar::onOpenFolderAction() { emit openFolderRequested(); }
void TMenuBar::onSaveAction() { emit saveRequested(); }
void TMenuBar::onSaveAsAction() { emit saveAsRequested(); }
void TMenuBar::onSettingsAction() { emit settingsRequest(); }
void TMenuBar::onExitApp() { emit exitRequested(); }
void TMenuBar::onRunAction() { emit runRequested(); }
void TMenuBar::onAboutAction() { emit aboutRequested(); }
void TMenuBar::onUpdateAction() {emit updateRequested(); }

