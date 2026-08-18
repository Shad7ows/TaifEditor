#pragma once

#include <QMenuBar>
#include <QFileSystemModel>
#include <QTreeView>



class TMenuBar : public QMenuBar {

	Q_OBJECT
public:
    TMenuBar(QWidget* parent = nullptr);

    QAction* newAction;
    QAction* openFileAction;
    QAction* openFolderAction;
    QAction* saveAction;
    QAction* saveAsAction;
    QAction* SettingsAction;
    QAction* exitAction;
    QAction* runAction;
    QAction* aboutAction;

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