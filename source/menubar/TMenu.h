#pragma once

#include <QMenuBar>
#include <QFileSystemModel>
#include <QTreeView>

class QAction;

class TMenuBar : public QMenuBar {

	Q_OBJECT
public:
        TMenuBar(QWidget* parent = nullptr);

    /** Updates View-menu checks to match the open/closed state of each dock. */
    void setOpenViewToolActions(bool alifOutputOpen,
                                bool terminalOpen,
                                bool problemsOpen);



    QAction* newAction = nullptr;
    QAction* openFileAction = nullptr;
    QAction* openFolderAction = nullptr;
    QAction* saveAction = nullptr;
    QAction* saveAsAction = nullptr;
    QAction* SettingsAction = nullptr;
    QAction* exitAction = nullptr;
    QAction* runAction = nullptr;
    QAction* aboutAction = nullptr;

    QAction* alifOutputAction = nullptr;
    QAction* terminalAction = nullptr;
        QAction* problemsAction = nullptr;

    QAction* undoAction = nullptr;
    QAction* redoAction = nullptr;
    QAction* cutAction = nullptr;
    QAction* copyAction = nullptr;
    QAction* pasteAction = nullptr;
    QAction* findAction = nullptr;
    QAction* replaceAction = nullptr;
    QAction* goToLineAction = nullptr;
    QAction* toggleCommentAction = nullptr;
    QAction* duplicateLineAction = nullptr;
    QAction* moveLineUpAction = nullptr;
    QAction* moveLineDownAction = nullptr;

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
