#pragma once

#include <QMenuBar>
#include <QFileSystemModel>
#include <QTreeView>



class TMenuBar : public QMenuBar {

	Q_OBJECT
public:
    TMenuBar(QWidget* parent = nullptr);

    /** Updates View-menu checks to match the open/closed state of each dock. */
    void setOpenViewToolActions(bool alifOutputOpen,
                                bool terminalOpen,
                                bool problemsOpen);

    QAction* newAction;
    QAction* openFileAction;
    QAction* openFolderAction;
    QAction* saveAction;
    QAction* saveAsAction;
    QAction* SettingsAction;
    QAction* exitAction;
    QAction* settingsAction;
    QAction* aboutAction;

    QAction* runAction;

    QAction* alifOutputAction;
    QAction* terminalAction;
    QAction* problemsAction;

    QAction* undoAction;
    QAction* redoAction;
    QAction* cutAction;
    QAction* copyAction;
    QAction* pasteAction;
    QAction* findAction;
    QAction* replaceAction;
    QAction* goToLineAction;
    QAction* toggleCommentAction;
    QAction* duplicateLineAction;
    QAction* moveLineUpAction;
    QAction* moveLineDownAction;

    QAction* updateAction;

signals:
    void newRequested();
    void openFileRequested();
    void openFolderRequested();
    void saveRequested();
    void saveAsRequested();
    void settingsRequest();
    void exitRequested();
    void runRequested();
    void aboutRequested();
    void updateRequested();

    void showAlifOutputRequested();
    void showTerminalRequested();
    void showProblemsRequested();

    void undoRequested();
    void redoRequested();
    void cutRequested();
    void copyRequested();
    void pasteRequested();
    void findRequested();
    void replaceRequested();
    void goToLineRequested();
    void toggleCommentRequested();
    void duplicateLineRequested();
    void moveLineUpRequested();
    void moveLineDownRequested();
};