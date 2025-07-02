#pragma once

#include <QMenuBar>
#include <QFileSystemModel>
#include <QTreeView>
#include <QProcess>



class SPMenuBar : public QMenuBar {

	Q_OBJECT
public:
    SPMenuBar(QWidget* parent = nullptr);


signals:
    void newRequested();
    void openRequested();
    void saveRequested();
    void saveAsRequested();
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
