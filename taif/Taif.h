#pragma once

//#include "TFolders.h"
#include "TEditor.h"
//#include "TTerminal.h"
#include "TMenu.h"
#include "TSettings.h"

#include <QMainWindow>


class Taif : public QMainWindow
{
    Q_OBJECT

public:
    Taif(const QString& filePath = "", QWidget* parent = nullptr);
    ~Taif();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile(QString);
    void saveFile();
    void saveFileAs();
    void openSettings();
    void exitApp();

    void runAlif();
    void aboutTaif();

    void updateWindowTitle();
    void onModificationChanged(bool modified);

private:
    int needSave();

private:
    TEditor* editor{};
    TMenuBar* menuBar{};
    TSettings* settings{};

    QString currentFilePath{};

};
