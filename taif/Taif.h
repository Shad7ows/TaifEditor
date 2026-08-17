#pragma once

#include "TEditor.h"
#include "TMenu.h"
#include "TSearchPanel.h"
#include "ProcessWorker.h"

#include <QMainWindow>
#include <QStatusBar>
#include <QSplitter>
#include <QFileSystemWatcher>


QT_BEGIN_NAMESPACE
namespace Ui { class Taif; }
QT_END_NAMESPACE

class Taif : public QMainWindow
{
    Q_OBJECT

public:
    Taif(const QString& filePath = "", QWidget* parent = nullptr);
    ~Taif();
    void loadFolder(const QString &folderPath);
protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
private slots:
    void newFile();
    void openFile(QString);
    void saveFile();
    void saveFileAs();
    void handleOpenFolderMenu();
    void openSettings();
    void exitApp();

    void runAlif();
    void aboutTaif();
    void checkForUpdates();

    void updateWindowTitle();
    void onModificationChanged(bool modified);
    void onFileTreeDoubleClicked(const QModelIndex &index);
    void closeTab(int index);
    void toggleSidebar();

    void toggleConsole();

    void updateCursorPosition();
    void onCurrentTabChanged();

    void onFileChanged(const QString &path);
    void showFindBar();
    void hideFindBar();
    void performSearch(bool forward, bool next);
    void findText();
    void findNextText();
    void findPrevText();
    void replaceOne();
    void replaceAll();
    void goToLine();
    void clearSearchHighlights();

private:
    void setupUI();
    void setupConnections();
    void setupStyle();
    int needSave();
    TEditor* currentEditor();
    void addWatch(const QString &filePath);
    void removeWatch(const QString &filePath);
    void reloadEditor(TEditor *editor);
    QString pathForEditor(TEditor *editor) const;
    // void setupTabWidget(QTabWidget* tw);
    // QTabWidget* tabWidgetForEditor(TEditor* editor) const;
    // QTabWidget* getTargetTabWidget();

private:
    QTabWidget *tabWidget{};
    TMenuBar* menuBar{};
    TSettings* setting{};
    QAction *toggleSidebarAction{};
    QString folderPath{};
    QAbstractItemModel* model{};

    QSplitter *mainSplitter{};
    QTreeView *fileTreeView{};
    QFileSystemModel *fileSystemModel{};

    QSplitter *editorSplitter{};
    QTabWidget *consoleTabWidget{};

    ProcessWorker* worker{};
    QThread* thread{};

    QFileSystemWatcher* fileWatcher{};
    bool savingFromApp{};
    QLabel *cursorPositionLabel{};
    // QLabel *encodingLabel{};
    // QProcess *alifProcess{};
    // QProcess *currentAlifProcess{};
    SearchPanel *searchBar{};
};
