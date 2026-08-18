#include "TMenu.h"

TMenuBar::TMenuBar(QWidget* parent) {
    QMenu* fileMenu = addMenu("ملف");
    QMenu* editMenu = addMenu("تحرير");
    QMenu* runMenu = addMenu("تشغيل");
    QMenu* helpMenu = addMenu("مساعدة");

    fileMenu->setMinimumWidth(200);
    editMenu->setMinimumWidth(200);
    runMenu->setMinimumWidth(200);
    helpMenu->setMinimumWidth(200);

    QAction* newAction = new QAction("جديد", parent);
    QAction* openFileAction = new QAction("فتح ملف", parent);
    QAction* openFolderAction = new QAction("فتح مجلد", parent);
    QAction* saveAction = new QAction("حفظ", parent);
    QAction* saveAsAction = new QAction("حفظ باسم", parent);
    QAction* settingsAction = new QAction("الإعدادات", parent);
    QAction* exitAction = new QAction("خروج", parent);

    QAction* undoAction = new QAction("تراجع", parent);
    QAction* redoAction = new QAction("إعادة", parent);
    QAction* cutAction = new QAction("قص", parent);
    QAction* copyAction = new QAction("نسخ", parent);
    QAction* pasteAction = new QAction("لصق", parent);
    QAction* findAction = new QAction("بحث", parent);
    QAction* replaceAction = new QAction("بحث واستبدال", parent);
    QAction* goToLineAction = new QAction("الذهاب إلى سطر", parent);
    QAction* toggleCommentAction = new QAction("تعليق سطر", parent);
    QAction* duplicateLineAction = new QAction("تكرار السطر", parent);
    QAction* moveLineUpAction = new QAction("نقل السطر لأعلى", parent);
    QAction* moveLineDownAction = new QAction("نقل السطر لأسفل", parent);

    // --- shortcuts --- //
    undoAction->setShortcut(QKeySequence::fromString("Ctrl+z"));


    QAction* runAction = new QAction("تشغيل", parent);

    QAction* aboutAction = new QAction("عن المحرر", parent);
    QAction* updateAction = new QAction("البحث عن تحديثات", parent);

    fileMenu->addAction(newAction);
    fileMenu->addAction(openFileAction);
    fileMenu->addAction(openFolderAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(settingsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    editMenu->addSeparator();
    editMenu->addAction(cutAction);
    editMenu->addAction(copyAction);
    editMenu->addAction(pasteAction);
    editMenu->addSeparator();
    editMenu->addAction(findAction);
    editMenu->addAction(replaceAction);
    editMenu->addAction(goToLineAction);
    editMenu->addSeparator();
    editMenu->addAction(toggleCommentAction);
    editMenu->addAction(duplicateLineAction);
    editMenu->addAction(moveLineUpAction);
    editMenu->addAction(moveLineDownAction);

    runMenu->addAction(runAction);

    helpMenu->addAction(aboutAction);
    helpMenu->addAction(updateAction);



    connect(newAction, &QAction::triggered, this, &TMenuBar::newRequested);
    connect(openFileAction, &QAction::triggered, this, &TMenuBar::openFileRequested);
    connect(openFolderAction, &QAction::triggered, this, &TMenuBar::openFolderRequested);
    connect(saveAction, &QAction::triggered, this, &TMenuBar::saveRequested);
    connect(saveAsAction, &QAction::triggered, this, &TMenuBar::saveAsRequested);
    connect(settingsAction, &QAction::triggered, this, &TMenuBar::settingsRequest);
    connect(exitAction, &QAction::triggered, this, &TMenuBar::exitRequested);

    connect(undoAction, &QAction::triggered, this, &TMenuBar::undoRequested);
    connect(redoAction, &QAction::triggered, this, &TMenuBar::redoRequested);
    connect(cutAction, &QAction::triggered, this, &TMenuBar::cutRequested);
    connect(copyAction, &QAction::triggered, this, &TMenuBar::copyRequested);
    connect(pasteAction, &QAction::triggered, this, &TMenuBar::pasteRequested);
    connect(findAction, &QAction::triggered, this, &TMenuBar::findRequested);
    connect(replaceAction, &QAction::triggered, this, &TMenuBar::replaceRequested);
    connect(goToLineAction, &QAction::triggered, this, &TMenuBar::goToLineRequested);
    connect(toggleCommentAction, &QAction::triggered, this, &TMenuBar::toggleCommentRequested);
    connect(duplicateLineAction, &QAction::triggered, this, &TMenuBar::duplicateLineRequested);
    connect(moveLineUpAction, &QAction::triggered, this, &TMenuBar::moveLineUpRequested);
    connect(moveLineDownAction, &QAction::triggered, this, &TMenuBar::moveLineDownRequested);

    connect(runAction, &QAction::triggered, this, &TMenuBar::runRequested);

    connect(aboutAction, &QAction::triggered, this, &TMenuBar::aboutRequested);
    connect(updateAction, &QAction::triggered, this, &TMenuBar::updateRequested);
}