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

};
