#include "TMenu.h"

#include <QAction>
#include <QMenu>
#include <QSignalBlocker>

TMenuBar::TMenuBar(QWidget* const parent)
    : QMenuBar(parent)
{
    QMenu* const fileMenu = addMenu(QStringLiteral("ملف"));
    QMenu* const viewMenu = addMenu(QStringLiteral("عرض"));
    QMenu* const runMenu = addMenu(QStringLiteral("تشغيل"));
    QMenu* const helpMenu = addMenu(QStringLiteral("مساعدة"));

    fileMenu->setMinimumWidth(200);
    viewMenu->setMinimumWidth(200);
    viewMenu->setLayoutDirection(Qt::RightToLeft);
    runMenu->setMinimumWidth(200);
    helpMenu->setMinimumWidth(200);

    newAction = new QAction(QStringLiteral("جديد"), this);
    openFileAction = new QAction(QStringLiteral("فتح ملف"), this);
    openFolderAction = new QAction(QStringLiteral("فتح مجلد"), this);
    saveAction = new QAction(QStringLiteral("حفظ"), this);
    saveAsAction = new QAction(QStringLiteral("حفظ باسم"), this);
    SettingsAction = new QAction(QStringLiteral("الإعدادات"), this);
    exitAction = new QAction(QStringLiteral("خروج"), this);

    runAction = new QAction(QStringLiteral("تشغيل"), this);

    alifOutputAction = new QAction(QStringLiteral("مخرجات ألف"), this);
    alifOutputAction->setObjectName(QStringLiteral("ShowAlifOutputAction"));
    alifOutputAction->setCheckable(true);
    terminalAction = new QAction(QStringLiteral("الطرفية"), this);
    terminalAction->setObjectName(QStringLiteral("ShowTerminalAction"));
    terminalAction->setCheckable(true);
    problemsAction = new QAction(QStringLiteral("الأخطاء"), this);
    problemsAction->setObjectName(QStringLiteral("ShowProblemsAction"));
    problemsAction->setCheckable(true);

    aboutAction = new QAction(QStringLiteral("عن المحرر"), this);
    QAction* const updateAction = new QAction(QStringLiteral("البحث عن تحديثات"), this);

    fileMenu->addAction(newAction);
    fileMenu->addAction(openFileAction);
    fileMenu->addAction(openFolderAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(SettingsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    viewMenu->addAction(alifOutputAction);
    viewMenu->addAction(terminalAction);
    viewMenu->addAction(problemsAction);

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
