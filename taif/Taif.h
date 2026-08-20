#pragma once

#include "TEditor.h"
#include "TMenu.h"
#include "TSearchPanel.h"
#include "ProcessWorker.h"
#include "SessionStore.h"

#include <QMainWindow>
#include <QStatusBar>
#include <QSplitter>


QT_BEGIN_NAMESPACE
namespace Ui { class Taif; }
QT_END_NAMESPACE

class QDockWidget;
class DiagnosticsPanel;
class TConsole;
class TBreadcrumbBar;

struct SessionRestoreResult final {
    QStringList openedFilePaths;
    QStringList unavailableFilePaths;
};

class Taif : public QMainWindow

{
    Q_OBJECT

public:
        Taif(const QString& filePath = "", QWidget* parent = nullptr,
         bool createInitialDocument = true);
    ~Taif();
    void loadFolder(const QString &folderPath);
    [[nodiscard]] SessionRestoreResult restoreSession(const SavedSession& session);

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



    void onFileTreeDoubleClicked(const QModelIndex &index);
    void closeTab(int index);
    void toggleSidebar();

    void toggleConsole();

    void updateCursorPosition();
    void onCurrentTabChanged();

    void showFindBar();
    void showReplaceBar();
    void hideFindBar();
    void performSearch(bool forward, bool next);
    void findText();
    void findNextText();
    void findPrevText();
    void replaceOne();
    void replaceAll();
    void goToLine();

private:
    void setupUI();

        void setupConnections();
    void connectSettingsSignals();
        void setupStyle();

    enum class SaveDecision : quint8 {
        Save,
        Discard,
        Cancel
    };

    [[nodiscard]] SaveDecision requestSaveDecision(TEditor* editor) const;
    [[nodiscard]] bool prepareEditorForClose(TEditor* editor);
    [[nodiscard]] bool saveEditor(TEditor* editor);
    [[nodiscard]] bool saveEditorAs(TEditor* editor);
    [[nodiscard]] bool writeEditorContents(TEditor* editor, const QString& filePath);
    void finalizeSavedEditor(TEditor* editor, const QString& filePath);
    void onEditorModificationChanged(TEditor* editor, bool modified);

    TEditor* currentEditor();

    void connectEditorDiagnostics(TEditor* editor);
    void refreshDiagnosticsPanel();
    void showAndRaiseDock(QDockWidget* dock);
    void syncBottomToolActionState();
    void clearSearchHighlights();
    void connectEditorActionState(TEditor* editor);
    void updateEditActionState();
    void refreshBreadcrumbs();
    void bindBreadcrumbsToEditor(TEditor* editor);
    void revealBreadcrumbPath(const QString& path);
    bool openDocumentFile(const QString& filePath, bool promptForBackupRecovery,
                          bool activateTab, bool updateRecentFiles,
                          QString* failureMessage = nullptr);
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
    TBreadcrumbBar* breadcrumbBar{};
    QMetaObject::Connection breadcrumbConnection{};
    QMetaObject::Connection cursorPositionConnection{};
    QDockWidget* terminalDock{};
    QDockWidget* alifOutputDock{};
    TConsole* systemTerminal{};
    TConsole* alifOutputConsole{};

    ProcessWorker* worker{};

    QThread* thread{};

    QLabel *cursorPositionLabel{};
    // QLabel *encodingLabel{};
    // QProcess *alifProcess{};
    // QProcess *currentAlifProcess{};
    SearchPanel *searchBar{};
    QDockWidget* diagnosticsDock{};
    DiagnosticsPanel* diagnosticsPanel{};
    bool openWelcomeAfterClose = false;
};
