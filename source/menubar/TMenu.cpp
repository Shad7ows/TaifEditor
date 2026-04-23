#include "TMenu.h"

TMenuBar::TMenuBar(QWidget* parent) {
    QMenu* fileMenu = addMenu("ملف");
    //QMenu* editMenu = addMenu("تحرير");
    QMenu* runMenu = addMenu("تشغيل");
    QMenu* helpMenu = addMenu("مساعدة");

    fileMenu->setMinimumWidth(200);
    //editMenu->setMinimumWidth(200);
    runMenu->setMinimumWidth(200);
    helpMenu->setMinimumWidth(200);

    QAction* newAction = new QAction("جديد", parent);
    QAction* openFileAction = new QAction("فتح ملف", parent);
    QAction* openFolderAction = new QAction("فتح مجلد", parent);
    QAction* saveAction = new QAction("حفظ", parent);
    QAction* saveAsAction = new QAction("حفظ باسم", parent);
    QAction* SettingsAction = new QAction("الإعدادات", parent);
    QAction* exitAction = new QAction("خروج", parent);

    QAction* runAction = new QAction("تشغيل", parent);

    QAction* aboutAction = new QAction("عن المحرر", parent);
    QAction* updateAction = new QAction("البحث عن تحديثات", parent);

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



    connect(newAction, &QAction::triggered, this, &TMenuBar::newRequested);
    connect(openFileAction, &QAction::triggered, this, &TMenuBar::openFileRequested);
    connect(openFolderAction, &QAction::triggered, this, &TMenuBar::openFolderRequested);
    connect(saveAction, &QAction::triggered, this, &TMenuBar::saveRequested);
    connect(saveAsAction, &QAction::triggered, this, &TMenuBar::saveAsRequested);
    connect(SettingsAction, &QAction::triggered, this, &TMenuBar::settingsRequest);
    connect(exitAction, &QAction::triggered, this, &TMenuBar::exitRequested);

    connect(runAction, &QAction::triggered, this, &TMenuBar::runRequested);

    connect(aboutAction, &QAction::triggered, this, &TMenuBar::aboutRequested);
    connect(updateAction, &QAction::triggered, this, &TMenuBar::updateRequested);
}

