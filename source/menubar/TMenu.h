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

    // أدوات تقسيم الشاشة
    QAction* splitHAction;
    QAction* splitVAction;

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
    void splitHRequested();
    void splitVRequested();
    void runRequested();
    void aboutRequested();
    void updateRequested();
};
