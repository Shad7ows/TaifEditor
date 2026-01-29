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

private slots:

    void onNewAction();
    void onOpenFileAction();
    void onOpenFolderAction();
    void onSaveAction();
    void onSaveAsAction();
    void onSettingsAction();
    void onExitApp();
    void onRunAction();
    void onAboutAction();
    void onUpdateAction();
};
