#pragma once

#include <QMenuBar>
#include <QFileSystemModel>
#include <QTreeView>
#include <QProcess>



class TMenuBar : public QMenuBar {

	Q_OBJECT
public:
    TMenuBar(QWidget* parent = nullptr);


signals:
    void newRequested();
    void openRequested();
    void saveRequested();
    void saveAsRequested();
    void settingsRequest();
    void exitRequested();
    void runRequested();
    void aboutRequested();

private slots:
    void onNewAction() {
        emit newRequested();
    }
    void onOpenAction() {
        emit openRequested();
    }
    void onSaveAction() {
        emit saveRequested();
    }
    void onSaveAsAction() {
        emit saveAsRequested();
    }
    void onSettingsAction() {
        emit settingsRequest();
    }
    void onExitApp() {
        emit exitRequested();
    }
    void onRunAction() {
        emit runRequested();
    }
    void onAboutAction() {
        emit aboutRequested();
    }
};
