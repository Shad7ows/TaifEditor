#include "TMenu.h"

#include <QAction>
#include <QMenu>
#include <QSignalBlocker>

TMenuBar::TMenuBar(QWidget* parent) {
    QMenu* const fileMenu = addMenu(QStringLiteral("ملف"));
    QMenu* const editMenu = addMenu(QStringLiteral("تحرير"));
    QMenu* const viewMenu = addMenu(QStringLiteral("عرض"));
    QMenu* const runMenu = addMenu(QStringLiteral("تشغيل"));
    QMenu* const helpMenu = addMenu(QStringLiteral("مساعدة"));

    fileMenu->setMinimumWidth(200);
    editMenu->setMinimumWidth(200);
    viewMenu->setMinimumWidth(200);
    runMenu->setMinimumWidth(200);
    helpMenu->setMinimumWidth(200);

    newAction = new QAction(QStringLiteral("جديد"), parent);
    openFileAction = new QAction(QStringLiteral("فتح ملف"), parent);
    openFolderAction = new QAction(QStringLiteral("فتح مجلد"), parent);
    saveAction = new QAction(QStringLiteral("حفظ"), parent);
    saveAsAction = new QAction(QStringLiteral("حفظ باسم"), parent);
    settingsAction = new QAction(QStringLiteral("الإعدادات"), parent);
    exitAction = new QAction(QStringLiteral("خروج"), parent);

    runAction = new QAction("تشغيل", parent);

    undoAction = new QAction(QStringLiteral("تراجع"), parent);
    redoAction = new QAction(QStringLiteral("إعادة"), parent);
    cutAction = new QAction(QStringLiteral("قص"), parent);
    copyAction = new QAction(QStringLiteral("نسخ"), parent);
    pasteAction = new QAction(QStringLiteral("لصق"), parent);
    findAction = new QAction(QStringLiteral("بحث"), parent);
    replaceAction = new QAction(QStringLiteral("بحث واستبدال"), parent);
    goToLineAction = new QAction(QStringLiteral("الذهاب إلى سطر"), parent);
    toggleCommentAction = new QAction(QStringLiteral("تعليق سطر"), parent);
    duplicateLineAction = new QAction(QStringLiteral("تكرار السطر"), parent);
    moveLineUpAction = new QAction(QStringLiteral("نقل السطر لأعلى"), parent);
    moveLineDownAction = new QAction(QStringLiteral("نقل السطر لأسفل"), parent);

    alifOutputAction = new QAction(QStringLiteral("مخرجات ألف"), this);
    alifOutputAction->setObjectName(QStringLiteral("ShowAlifOutputAction"));
    alifOutputAction->setCheckable(true);
    terminalAction = new QAction(QStringLiteral("الطرفية"), this);
    terminalAction->setObjectName(QStringLiteral("ShowTerminalAction"));
    terminalAction->setCheckable(true);
    problemsAction = new QAction(QStringLiteral("الأخطاء"), this);
    problemsAction->setObjectName(QStringLiteral("ShowProblemsAction"));
    problemsAction->setCheckable(true);

    aboutAction = new QAction(QStringLiteral("عن المحرر"), parent);
    updateAction = new QAction(QStringLiteral("البحث عن تحديثات"), parent);



    // --- shortcuts --- //
    undoAction->setShortcut(QKeySequence::fromString("Ctrl+z"));




    fileMenu->addAction(newAction);
    fileMenu->addAction(openFileAction);
    fileMenu->addAction(openFolderAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(settingsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    runMenu->addAction(runAction);

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

    viewMenu->addAction(alifOutputAction);
    viewMenu->addAction(terminalAction);
    viewMenu->addAction(problemsAction);

    helpMenu->addAction(aboutAction);
    helpMenu->addAction(pasteAction);



    connect(newAction, &QAction::triggered, this, &TMenuBar::newRequested);
    connect(openFileAction, &QAction::triggered, this, &TMenuBar::openFileRequested);
    connect(openFolderAction, &QAction::triggered, this, &TMenuBar::openFolderRequested);
    connect(saveAction, &QAction::triggered, this, &TMenuBar::saveRequested);
    connect(saveAsAction, &QAction::triggered, this, &TMenuBar::saveAsRequested);
    connect(settingsAction, &QAction::triggered, this, &TMenuBar::settingsRequest);
    connect(exitAction, &QAction::triggered, this, &TMenuBar::exitRequested);

    connect(runAction, &QAction::triggered, this, &TMenuBar::runRequested);

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

    connect(alifOutputAction, &QAction::triggered, this, &TMenuBar::showAlifOutputRequested);
    connect(terminalAction, &QAction::triggered, this, &TMenuBar::showTerminalRequested);
    connect(problemsAction, &QAction::triggered, this, &TMenuBar::showProblemsRequested);

    connect(aboutAction, &QAction::triggered, this, &TMenuBar::aboutRequested);
    connect(updateAction, &QAction::triggered, this, &TMenuBar::updateRequested);
}


void TMenuBar::setOpenViewToolActions(const bool alifOutputOpen,
                                      const bool terminalOpen,
                                      const bool problemsOpen)
{
    const QSignalBlocker alifOutputActionBlocker(alifOutputAction);
    const QSignalBlocker terminalActionBlocker(terminalAction);
    const QSignalBlocker problemsActionBlocker(problemsAction);

    if (alifOutputAction != nullptr) {
        alifOutputAction->setChecked(alifOutputOpen);
    }
    if (terminalAction != nullptr) {
        terminalAction->setChecked(terminalOpen);
    }
    if (problemsAction != nullptr) {
        problemsAction->setChecked(problemsOpen);
    }
}