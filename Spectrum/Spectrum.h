#pragma once

//#include "SPFolders.h"
#include "SPEditor.h"
//#include "SPTerminal.h"
#include "SPMenu.h"
#include "SPSettings.h"

#include <QMainWindow>


class Spectrum : public QMainWindow
{
    Q_OBJECT

public:
    Spectrum(const QString& filePath = "", QWidget* parent = nullptr);
    ~Spectrum();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile(QString);
    void saveFile();
    void saveFileAs();
    void exitApp();

    void runAlif();
    void aboutSpectrum();

    void updateWindowTitle();
    void onModificationChanged(bool modified);

private:
    int needSave();

private:
    SPEditor* editor{};
    SPMenuBar* menuBar{};

    QString currentFilePath{};




private slots:
    void openSettings() {
        // Prevent multiple settings windows
        if (settingsWindow) return;

        settingsWindow = new SPSettings(this);
        connect(settingsWindow, &SPSettings::settingsChanged, this, &Spectrum::applySettings);
        connect(settingsWindow, &SPSettings::windowClosed, this, [this]() {
            settingsWindow->deleteLater();
            settingsWindow = nullptr;
        });

        settingsWindow->show();
    }

    void applySettings() {
        // In a real application, you would implement actual settings application
        qDebug() << "Settings changed - reloading application style...";
        // Example: QApplication::setStyle(QStyleFactory::create("Fusion"));
    }

private:
    void centerWindow() {
        QRect screenGeometry = screen()->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }

    SPSettings* settingsWindow = nullptr;
};
