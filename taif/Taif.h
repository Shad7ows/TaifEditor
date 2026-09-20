#pragma once

#include "TEditor.h"
#include "TMenu.h"
#include "TSearchPanel.h"
#include "AlifRunController.h"
#include "SessionStore.h"
#include "RecoveryCoordinator.h"

#include <QMainWindow>
#include <QStatusBar>
#include <QSplitter>
#include <QFileSystemWatcher>
#include <QTimer>


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
signals:
    void returnToWelcomeRequested();
    void closeRejected();

private slots:
    void onFileChanged(const QString &path);
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
    void clearSearchHighlights();
    void refreshBreadcrumbs();
    void bindBreadcrumbsToEditor(TEditor* editor);
    void revealBreadcrumbPath(const QString& path);

private:
    void setupUI();
    void connectSettingsSignals();
    void setupConnections();
    void applyEditorPreferences(const EditorPreferences& preferences);
    void setupStyle();
    void setupTimers();

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
    void registerEditorRecovery(TEditor* editor);
    void flushRecoverySnapshots();
    void importKnownLegacyRecoveryEntries(const QString& launchFilePath);
    void presentRecoveryEntries();
    void restoreRecoveryEntry(const RecoveryEntry& entry);

    TEditor* currentEditor();

    void connectEditorDiagnostics(TEditor* editor);
    void refreshDiagnosticsPanel();
    void showAndRaiseDock(QDockWidget* dock);
    void syncBottomToolActionState();
    void addWatch(const QString &filePath);
    void removeWatch(const QString &filePath);
    void reloadEditor(TEditor *editor);
    QString pathForEditor(TEditor *editor) const;
    bool openDocumentFile(const QString& filePath, bool promptForBackupRecovery,
                          bool activateTab, bool updateRecentFiles,
                          QString* failureMessage = nullptr);

private:
    QTabWidget *tabWidget{};
    TMenuBar* menuBar{};
    TSettings* setting{};
    RecoveryCoordinator* recoveryCoordinator{};
    bool recoveryCloseFlushPending = false;
    bool recoveryCloseFlushAcknowledged = false;
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

    AlifRunController* runController{};
    QAction* runToolbarAction{};

    QFileSystemWatcher* fileWatcher{};
    bool savingFromApp{};
    QTimer *saveSuppressTimer{};
    QLabel *cursorPositionLabel{};
    SearchPanel *searchBar{};
    QDockWidget* diagnosticsDock{};
    DiagnosticsPanel* diagnosticsPanel{};
};
