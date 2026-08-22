#include "TMenu.h"

#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QSignalBlocker>

namespace {

QAction* createEditAction(QObject* const parent,
                          const QString& text,
                          const QString& objectName,
                          const QKeySequence& shortcut)
{
    auto* const action = new QAction(text, parent);
    action->setObjectName(objectName);
    action->setShortcut(shortcut);
    return action;
}

} // namespace

TMenuBar::TMenuBar(QWidget* const parent)
    : QMenuBar(parent)
{
    QMenu* const fileMenu = addMenu(QStringLiteral("ملف"));
    QMenu* const editMenu = addMenu(QStringLiteral("تحرير"));
    QMenu* const viewMenu = addMenu(QStringLiteral("عرض"));
    QMenu* const runMenu = addMenu(QStringLiteral("تشغيل"));
    QMenu* const helpMenu = addMenu(QStringLiteral("مساعدة"));

    fileMenu->setMinimumWidth(200);
    editMenu->setMinimumWidth(240);
    editMenu->setLayoutDirection(Qt::RightToLeft);
    viewMenu->setMinimumWidth(200);
    viewMenu->setLayoutDirection(Qt::RightToLeft);
    runMenu->setMinimumWidth(200);
    helpMenu->setMinimumWidth(200);

    newAction = new QAction(QStringLiteral("جديد"), this);
    openFileAction = new QAction(QStringLiteral("فتح ملف"), this);
    openFolderAction = new QAction(QStringLiteral("فتح مجلد"), this);
    saveAction = new QAction(QStringLiteral("حفظ"), this);
    saveAction->setShortcut(QKeySequence::Save);
    saveAsAction = new QAction(QStringLiteral("حفظ باسم"), this);
    SettingsAction = new QAction(QStringLiteral("الإعدادات"), this);
    exitAction = new QAction(QStringLiteral("خروج"), this);

    undoAction = createEditAction(this, QStringLiteral("تراجع"),
                                  QStringLiteral("UndoAction"), QKeySequence::Undo);
    redoAction = createEditAction(this, QStringLiteral("إعادة"),
                                  QStringLiteral("RedoAction"), QKeySequence::Redo);
    cutAction = createEditAction(this, QStringLiteral("قص"),
                                 QStringLiteral("CutAction"), QKeySequence::Cut);
    copyAction = createEditAction(this, QStringLiteral("نسخ"),
                                  QStringLiteral("CopyAction"), QKeySequence::Copy);
    pasteAction = createEditAction(this, QStringLiteral("لصق"),
                                   QStringLiteral("PasteAction"), QKeySequence::Paste);
    findAction = createEditAction(this, QStringLiteral("بحث"),
                                  QStringLiteral("FindAction"), QKeySequence::Find);
    replaceAction = createEditAction(this, QStringLiteral("بحث واستبدال"),
                                     QStringLiteral("ReplaceAction"),
                                     QKeySequence(QStringLiteral("Ctrl+H")));
    goToLineAction = createEditAction(this, QStringLiteral("الذهاب إلى سطر"),
                                      QStringLiteral("GoToLineAction"),
                                      QKeySequence(QStringLiteral("Ctrl+G")));
    toggleCommentAction = createEditAction(this, QStringLiteral("تعليق سطر"),
                                            QStringLiteral("ToggleCommentAction"),
                                            QKeySequence(QStringLiteral("Ctrl+/")));
    duplicateLineAction = createEditAction(this, QStringLiteral("تكرار السطر"),
                                            QStringLiteral("DuplicateLineAction"),
                                            QKeySequence(QStringLiteral("Ctrl+D")));
    moveLineUpAction = createEditAction(this, QStringLiteral("نقل السطر لأعلى"),
                                         QStringLiteral("MoveLineUpAction"),
                                         QKeySequence(QStringLiteral("Alt+Up")));
    moveLineDownAction = createEditAction(this, QStringLiteral("نقل السطر لأسفل"),
                                           QStringLiteral("MoveLineDownAction"),
                                           QKeySequence(QStringLiteral("Alt+Down")));

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
    aiAssistantAction = new QAction(QStringLiteral("مساعد الذكاء الاصطناعي"), this);
    aiAssistantAction->setObjectName(QStringLiteral("ShowAiAssistantAction"));
    aiAssistantAction->setCheckable(true);

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
    viewMenu->addAction(aiAssistantAction);

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
    connect(alifOutputAction, &QAction::triggered, this, &TMenuBar::showAlifOutputRequested);
    connect(terminalAction, &QAction::triggered, this, &TMenuBar::showTerminalRequested);
    connect(problemsAction, &QAction::triggered, this, &TMenuBar::showProblemsRequested);
    connect(aiAssistantAction, &QAction::triggered, this, &TMenuBar::showAiAssistantRequested);

    connect(aboutAction, &QAction::triggered, this, &TMenuBar::aboutRequested);
    connect(updateAction, &QAction::triggered, this, &TMenuBar::updateRequested);
}

void TMenuBar::setOpenViewToolActions(const bool alifOutputOpen,
                                      const bool terminalOpen,
                                      const bool problemsOpen,
                                      const bool aiAssistantOpen)
{
    const QSignalBlocker alifOutputActionBlocker(alifOutputAction);
    const QSignalBlocker terminalActionBlocker(terminalAction);
    const QSignalBlocker problemsActionBlocker(problemsAction);
    const QSignalBlocker aiAssistantActionBlocker(aiAssistantAction);

    if (alifOutputAction != nullptr) {
        alifOutputAction->setChecked(alifOutputOpen);
    }
    if (terminalAction != nullptr) {
        terminalAction->setChecked(terminalOpen);
    }
    if (problemsAction != nullptr) {
        problemsAction->setChecked(problemsOpen);
    }
    if (aiAssistantAction != nullptr) {
        aiAssistantAction->setChecked(aiAssistantOpen);
    }
}
