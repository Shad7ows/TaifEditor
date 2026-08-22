#include "Taif.h"
#include "TWelcomeWindow.h"
#include "TConsole.h"
#include "InlinePromptConsole.h"
#include "DockableConsoleTool.h"

#include "TSearchPanel.h"
#include "SearchReplaceEngine.h"
#include "DiagnosticsPanel.h"
#include "TBreadcrumbBar.h"
#include "EditorInfoBar.h"
#include "ProjectExplorerWidget.h"
#include "ProjectFileOperations.h"
#include "GitPanelWidget.h"
#include "GitRepositoryService.h"
#include "TRecoveryDialog.h"


#include <QDockWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

#include <QShortcut>
#include <QGuiApplication>
#include <QScreen>
#include <QCoreApplication>
#include <QTextStream>
#include <QApplication>
#include <QClipboard>

#include <QToolBar>
#include <QHeaderView>
#include <QSettings>
#include <QProcess>
#include <QStyleFactory>
#include <QKeyEvent>
#include <QTimer>
#include <QInputDialog>
#include <QTextEdit>
#include <QMimeData>
#include <QSet>
#include <QSaveFile>
#include <QStatusBar>

Taif::Taif(const QString& filePath, QWidget* const parent,
           const bool createInitialDocument)

    : QMainWindow(parent)
{

    setAttribute(Qt::WA_DeleteOnClose);

    setting = new TSettings(this);
    recoveryCoordinator = new RecoveryCoordinator({}, this);
    recoveryCoordinator->pruneExpiredEntries(30);

    setupUI();
    runController = new AlifRunController(this);
    connect(runController, &AlifRunController::standardOutput,
            alifOutputConsole, &TConsole::appendPlainTextThreadSafe);
    connect(runController, &AlifRunController::standardError,
            alifOutputConsole, &TConsole::appendPlainTextThreadSafe);
    connect(runController, &AlifRunController::launchFailed, this, [this](const QString& message) {
        if (alifOutputConsole != nullptr) {
            alifOutputConsole->appendPlainTextThreadSafe(
                QStringLiteral("تعذر تشغيل ألف: %1\n").arg(message));
        }
    });
    connect(runController, &AlifRunController::finished, this,
            [this](const int exitCode, const QProcess::ExitStatus exitStatus) {
        if (alifOutputConsole == nullptr) {
            return;
        }
        const QString outcome = exitStatus == QProcess::NormalExit
            ? QStringLiteral("اكتمل التنفيذ (رمز الخروج = %1).\n").arg(exitCode)
            : QStringLiteral("انتهى التنفيذ بشكل غير طبيعي.\n");
        alifOutputConsole->appendPlainTextThreadSafe(
            QStringLiteral("\n──────────────────────────────\n%1").arg(outcome));
        if (auto* const inlineConsole = qobject_cast<InlinePromptConsole*>(alifOutputConsole)) {
            inlineConsole->endInput();
        }
    });
    connect(runController, &AlifRunController::stateChanged, this,
            [this](const AlifRunController::State state) {
        const bool active = state == AlifRunController::State::Starting
            || state == AlifRunController::State::Running
            || state == AlifRunController::State::Stopping;
        const QString label = active ? QStringLiteral("إيقاف التنفيذ")
                                     : QStringLiteral("تشغيل");
        if (auto* const inlineConsole = qobject_cast<InlinePromptConsole*>(alifOutputConsole)) {
            if (active) {
                inlineConsole->beginInput();
            } else {
                inlineConsole->endInput();
            }
        }
        if (menuBar != nullptr && menuBar->runAction != nullptr) {
            menuBar->runAction->setText(label);
        }
        if (runToolbarAction != nullptr) {
            runToolbarAction->setToolTip(label);
        }
    });
    connect(alifOutputConsole, &TConsole::commandEntered,
            runController, &AlifRunController::sendInput);
    setupConnections();
    connectSettingsSignals();
    setupStyle();

    connect(recoveryCoordinator, &RecoveryCoordinator::removalFailed, this,
            [this](const QString&, const QString& errorMessage) {
                statusBar()->showMessage(errorMessage.isEmpty()
                    ? QStringLiteral("تعذر حذف نسخة الاستعادة.") : errorMessage, 7000);
            });
    connect(recoveryCoordinator, &RecoveryCoordinator::flushCompleted, this,
            [this](const bool allPersisted) {
                if (!recoveryCloseFlushPending) {
                    return;
                }
                recoveryCloseFlushPending = false;
                if (!allPersisted) {
                    setEnabled(true);
                    statusBar()->showMessage(
                        QStringLiteral("تعذر تأكيد حفظ نسخة الاستعادة. أعد المحاولة قبل الإغلاق."),
                        7000);
                    emit closeRejected();
                    return;
                }

                recoveryCloseFlushAcknowledged = true;
                close();
            });

    installEventFilter(this);
    // Importing legacy adjacent backups can involve disk reads and atomic writes.
    // Defer it until the event loop begins so construction and first paint stay responsive.
    QTimer::singleShot(0, this, [this, filePath]() {
        importKnownLegacyRecoveryEntries(filePath);
        presentRecoveryEntries();
    });

    if (!filePath.isEmpty()) {
        QString failureMessage;
        if (!openDocumentFile(filePath, true, true, true, &failureMessage)) {
            QMessageBox::warning(this, QStringLiteral("خطأ"),
                                 failureMessage.isEmpty()
                                     ? QStringLiteral("لا يمكن فتح الملف.") : failureMessage);
        }
    } else if (createInitialDocument && tabWidget->count() == 0) {
        newFile();
    }

}

Taif::~Taif() {
    if (runController != nullptr) {
        runController->shutdown();
    }

    if (TEditor* editor = currentEditor()) {
        QSettings settings("Alif", "Taif");
        settings.setValue("editorFontSize", editor->font().pixelSize());
        settings.setValue("editorFontType", editor->font().family());
        settings.setValue("editorCodeTheme", setting->getThemeCombo()->currentIndex());
        settings.sync();
    }
}

void Taif::setupUI() {

    tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("MainTabs");
    tabWidget->setDocumentMode(true);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);

    menuBar = new TMenuBar(this);
    setMenuBar(menuBar);

        mainSplitter = new QSplitter(Qt::Horizontal, this);
    projectExplorer = new ProjectExplorerWidget(this);
    projectExplorer->setVisible(false);

    editorSplitter = new QSplitter(Qt::Vertical, this);
    searchBar = new SearchPanel(this);
    searchBar->hide();

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeo = screen->availableGeometry();
    int margin = 100;
    int widthFixedNum = 6;
    int x = screenGeo.right() - screenGeo.size().width() + margin * widthFixedNum / 2;
    int y = screenGeo.top() + 30 + margin / 2; // 30 is top system bar height
    int width = screenGeo.size().width() - margin * widthFixedNum;
    int height = screenGeo.size().height() - margin;
    this->setGeometry(x, y, width, height);

    QToolBar *mainToolBar = new QToolBar("Main Toolbar", this);
    mainToolBar->setObjectName("mainToolBar");
    mainToolBar->setMovable(false);
    mainToolBar->setIconSize(QSize(25, 25));
    this->addToolBar(Qt::RightToolBarArea, mainToolBar);

    toggleSidebarAction = new QAction(QIcon(":/icons/resources/panel-right-open.svg"), "فتح القائمة", this);
    toggleSidebarAction->setCheckable(true);
    toggleSidebarAction->setChecked(false);
    mainToolBar->addAction(toggleSidebarAction);

    runToolbarAction = new QAction(QIcon(":/icons/resources/run.svg"), "تشغيل الملف الحالي", this);

    mainToolBar->addAction(runToolbarAction);
        connect(runToolbarAction, &QAction::triggered, this, &Taif::runAlif);

    auto* const editorPane = new QWidget(editorSplitter);
    auto* const editorPaneLayout = new QVBoxLayout(editorPane);
    editorPaneLayout->setContentsMargins(0, 0, 0, 0);
    editorPaneLayout->setSpacing(0);
    breadcrumbBar = new TBreadcrumbBar(editorPane);
    editorPaneLayout->addWidget(breadcrumbBar);
    editorPaneLayout->addWidget(tabWidget, 1);
    editorSplitter->addWidget(editorPane);
    editorSplitter->setSizes({1000});

    mainSplitter->addWidget(projectExplorer);
    mainSplitter->addWidget(editorSplitter);
    mainSplitter->setSizes({200, 700});
    this->setCentralWidget(mainSplitter);

    diagnosticsDock = new QDockWidget(QStringLiteral("المشكلات"), this);
    diagnosticsDock->setObjectName(QStringLiteral("DiagnosticsDock"));
    diagnosticsDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    diagnosticsDock->setFeatures(QDockWidget::DockWidgetMovable
                                 | QDockWidget::DockWidgetFloatable
                                 | QDockWidget::DockWidgetClosable);
    diagnosticsPanel = new DiagnosticsPanel(diagnosticsDock);
    diagnosticsDock->setWidget(diagnosticsPanel);
    addDockWidget(Qt::BottomDockWidgetArea, diagnosticsDock);

    gitPanel = new GitPanelWidget(projectExplorer->gitRepositoryService(), this);
    gitDock = new QDockWidget(QStringLiteral("Git"), this);
    gitDock->setObjectName(QStringLiteral("GitDock"));
    gitDock->setWidget(gitPanel);
    gitDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    gitDock->setStyleSheet(QStringLiteral(R"(
        QDockWidget#GitDock { background:#0f172a; color:#e2e8f0; border:1px solid #1e3a5f; }
        QDockWidget#GitDock::title { background:#111d33; color:#dbeafe; padding:7px 9px; border-bottom:1px solid #263a57; text-align:right; }
        QDockWidget#GitDock::close-button, QDockWidget#GitDock::float-button { background:transparent; border:none; }
        QDockWidget#GitDock::close-button:hover, QDockWidget#GitDock::float-button:hover { background:#1e3a5f; }
    )"));
    addDockWidget(Qt::RightDockWidgetArea, gitDock);
    gitDock->hide();

    auto* const gitEdgeToolbar = new QToolBar(QStringLiteral("Git"), this);
    gitEdgeToolbar->setObjectName(QStringLiteral("GitEdgeToolbar"));
    gitEdgeToolbar->setOrientation(Qt::Vertical);
    gitEdgeToolbar->setStyleSheet(QStringLiteral(R"(
        QToolBar#GitEdgeToolbar { background:#0f172a; border-left:1px solid #1e3a5f; spacing:3px; padding:3px; }
        QToolButton { background:#111d33; color:#93c5fd; border:1px solid #263a57; border-radius:4px; padding:6px 4px; }
        QToolButton:hover, QToolButton:checked { background:#1e3a5f; color:#ffffff; border-color:#3b82f6; }
    )"));
    showGitPanelAction = gitEdgeToolbar->addAction(QStringLiteral("Git"));
    showGitPanelAction->setCheckable(true);
    showGitPanelAction->setToolTip(QStringLiteral("إظهار لوحة Git"));
    addToolBar(Qt::RightToolBarArea, gitEdgeToolbar);
    resizeDocks({diagnosticsDock}, {220}, Qt::Vertical);
    connect(diagnosticsPanel, &DiagnosticsPanel::diagnosticActivated,
            this, [this](const EditorDiagnostic& diagnostic) {
                if (TEditor* editor = currentEditor()) {
                    editor->navigateToDiagnosticRange(diagnostic.range);
                }
            });

#if defined(Q_OS_LINUX)
    const QString terminalTitle = QStringLiteral("طرفية النظام (Bash)");
#elif defined(Q_OS_MACOS)
    const QString terminalTitle = QStringLiteral("طرفية النظام (Zsh)");
#else
    const QString terminalTitle = QStringLiteral("طرفية النظام (CMD)");
#endif
    const DockableConsoleTool terminalTool = DockableConsoleToolFactory::create(
        this, terminalTitle, QStringLiteral("TerminalDock"),
        QStringLiteral("SystemTerminalConsole"), true);
    terminalDock = terminalTool.dock;
    systemTerminal = terminalTool.console;

    const DockableConsoleTool outputTool = DockableConsoleToolFactory::create(
        this, QStringLiteral("مخرجات ألف"), QStringLiteral("AlifOutputDock"),
        QStringLiteral("AlifOutputConsole"), false);
    alifOutputDock = outputTool.dock;
    alifOutputConsole = outputTool.console;

    if (terminalDock && alifOutputDock) {
        DockableConsoleToolFactory::ensureTabifiedWith(this, diagnosticsDock, alifOutputDock);
        DockableConsoleToolFactory::ensureTabifiedWith(this, diagnosticsDock, terminalDock);
        terminalDock->hide();
        alifOutputDock->hide();
    }

    editorInfoBar = new EditorInfoBar(statusBar());
    statusBar()->addPermanentWidget(editorInfoBar, 1);
    connect(editorInfoBar, &EditorInfoBar::diagnosticsActivated, this,
            [this]() { showAndRaiseDock(diagnosticsDock); });
}

void Taif::setupConnections() {

    connect(projectExplorer, &ProjectExplorerWidget::fileActivationRequested,
            this, &Taif::onProjectFileActivated);
    connect(projectExplorer, &ProjectExplorerWidget::createFileRequested,
            this, &Taif::createProjectFile);
    connect(projectExplorer, &ProjectExplorerWidget::createFolderRequested,
            this, &Taif::createProjectFolder);
    connect(projectExplorer, &ProjectExplorerWidget::renameRequested,
            this, &Taif::renameProjectPath);
    connect(projectExplorer, &ProjectExplorerWidget::deleteRequested,
            this, &Taif::deleteProjectPath);
    connect(projectExplorer, &ProjectExplorerWidget::revealRequested,
            this, &Taif::revealProjectPath);
    connect(showGitPanelAction, &QAction::triggered, this, &Taif::showGitPanel);
    connect(gitDock, &QDockWidget::visibilityChanged, this,
            [this](const bool visible) { showGitPanelAction->setChecked(visible); });
    connect(gitPanel, &GitPanelWidget::destructiveOperationRequested,
            this, &Taif::handleGitDestructiveOperation);
    connect(gitPanel, &GitPanelWidget::pullRequested, this, &Taif::handleGitPull);
    connect(gitPanel, &GitPanelWidget::branchSwitchRequested, this, &Taif::handleGitBranchSwitch);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &Taif::closeTab);
    connect(toggleSidebarAction, &QAction::triggered, this, &Taif::toggleSidebar);

    connect(menuBar, &TMenuBar::newRequested, this, &Taif::newFile);
    connect(menuBar, &TMenuBar::openFileRequested, this, [this](){this->openFile("");});
    connect(menuBar, &TMenuBar::saveRequested, this, &Taif::saveFile);
    connect(menuBar, &TMenuBar::saveAsRequested, this, &Taif::saveFileAs);
    connect(menuBar, &TMenuBar::settingsRequest, this, &Taif::openSettings);
        connect(menuBar, &TMenuBar::exitRequested, this, &Taif::exitApp);
    connect(menuBar, &TMenuBar::undoRequested, this,
            [this]() { if (TEditor* editor = currentEditor()) editor->undo(); });
    connect(menuBar, &TMenuBar::redoRequested, this,
            [this]() { if (TEditor* editor = currentEditor()) editor->redo(); });
    connect(menuBar, &TMenuBar::cutRequested, this,
            [this]() { if (TEditor* editor = currentEditor()) editor->cut(); });
    connect(menuBar, &TMenuBar::copyRequested, this,
            [this]() { if (TEditor* editor = currentEditor()) editor->copy(); });
    connect(menuBar, &TMenuBar::pasteRequested, this,
            [this]() { if (TEditor* editor = currentEditor()) editor->paste(); });
    connect(menuBar, &TMenuBar::findRequested, this, &Taif::showFindBar);
    connect(menuBar, &TMenuBar::replaceRequested, this, &Taif::showReplaceBar);
    connect(menuBar, &TMenuBar::goToLineRequested, this, &Taif::goToLine);
    connect(menuBar, &TMenuBar::toggleCommentRequested, this,
            [this]() { if (TEditor* editor = currentEditor()) editor->toggleComment(); });
    connect(menuBar, &TMenuBar::duplicateLineRequested, this,
            [this]() { if (TEditor* editor = currentEditor()) editor->duplicateLine(); });
    connect(menuBar, &TMenuBar::moveLineUpRequested, this,
            [this]() { if (TEditor* editor = currentEditor()) editor->moveLineUp(); });
    connect(menuBar, &TMenuBar::moveLineDownRequested, this,
            [this]() { if (TEditor* editor = currentEditor()) editor->moveLineDown(); });
    connect(menuBar, &TMenuBar::runRequested, this, &Taif::runAlif);

    connect(menuBar, &TMenuBar::aboutRequested, this, &Taif::aboutTaif);
    connect(menuBar, &TMenuBar::updateRequested, this, &Taif::checkForUpdates);
        connect(menuBar, &TMenuBar::openFolderRequested, this, &Taif::handleOpenFolderMenu);
    connect(menuBar, &TMenuBar::showAlifOutputRequested, this,
            [this]() { showAndRaiseDock(alifOutputDock); });
    connect(menuBar, &TMenuBar::showTerminalRequested, this,
            [this]() { showAndRaiseDock(terminalDock); });
    connect(menuBar, &TMenuBar::showProblemsRequested, this,
            [this]() { showAndRaiseDock(diagnosticsDock); });

    const auto scheduleBottomToolActionStateSync = [this](const bool) {
        QTimer::singleShot(0, this, &Taif::syncBottomToolActionState);
    };
    connect(diagnosticsDock, &QDockWidget::visibilityChanged,
            this, scheduleBottomToolActionStateSync);
    connect(alifOutputDock, &QDockWidget::visibilityChanged,
            this, scheduleBottomToolActionStateSync);
    connect(terminalDock, &QDockWidget::visibilityChanged,
            this, scheduleBottomToolActionStateSync);
    syncBottomToolActionState();
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &Taif::updateEditActionState);

    connect(tabWidget, &QTabWidget::currentChanged, this, &Taif::updateWindowTitle);

    connect(tabWidget, &QTabWidget::currentChanged, this, &Taif::onCurrentTabChanged);
    connect(breadcrumbBar, &TBreadcrumbBar::fileSegmentActivated,
            this, &Taif::revealBreadcrumbPath);
    connect(breadcrumbBar, &TBreadcrumbBar::symbolSegmentActivated, this,
            [this](const SourceRange range) {
                if (TEditor* const editor = currentEditor()) {
                    editor->navigateToDiagnosticRange(range);
                }
            });

    connect(searchBar, &SearchPanel::findNext, this, &Taif::findNextText);
    connect(searchBar, &SearchPanel::findText, this, &Taif::findText);
        connect(searchBar, &SearchPanel::findPrevious, this, &Taif::findPrevText);
    connect(searchBar, &SearchPanel::replaceOne, this, &Taif::replaceOne);
    connect(searchBar, &SearchPanel::replaceAll, this, &Taif::replaceAll);
    connect(searchBar, &SearchPanel::closed, this, &Taif::hideFindBar);


}

void Taif::connectSettingsSignals()
{
    if (setting == nullptr) {
        return;
    }

    const auto applyPreferences = [this](const EditorPreferences& preferences) {
        applyEditorPreferences(preferences);
    };
    connect(setting, &TSettings::preferencesPreviewed, this, applyPreferences);
    connect(setting, &TSettings::preferencesApplied, this, applyPreferences);
}

void Taif::applyEditorPreferences(const EditorPreferences& requestedPreferences)
{
    if (setting == nullptr) {
        return;
    }

    const EditorPreferences preferences = PreferencesStore::normalize(requestedPreferences);
    const QVector<std::shared_ptr<SyntaxTheme>> themes = setting->getAvailableThemes();
    const std::shared_ptr<SyntaxTheme> theme = themes.isEmpty()
        ? std::shared_ptr<SyntaxTheme>()
        : themes.at(qBound(0, preferences.syntaxThemeIndex, themes.size() - 1));

    for (int index = 0; index < tabWidget->count(); ++index) {
        if (auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index))) {
            editor->applyPreferences(preferences);
            if (theme) {
                editor->updateHighlighterTheme(theme);
            }
        }
    }
}

void Taif::setupStyle() {

    QString styleSheet = R"(
        QMainWindow {
            background-color: #0f172a;
            font-size: 13px;
        }

        QDockWidget#DiagnosticsDock,
        QDockWidget#TerminalDock,
        QDockWidget#AlifOutputDock {
            background-color: #0f172a;
            color: #e2e8f0;
            font-family: "Tajawal", "Noto Kufi Arabic";
            border-top: 1px solid #334155;
        }
        QDockWidget#DiagnosticsDock::title,
        QDockWidget#TerminalDock::title,
        QDockWidget#AlifOutputDock::title {
            background-color: #1e293b;
            color: #e2e8f0;
            text-align: right;
            padding: 7px 10px;
            border-bottom: 1px solid #334155;
        }
        QDockWidget#DiagnosticsDock::close-button,
        QDockWidget#DiagnosticsDock::float-button,
        QDockWidget#TerminalDock::close-button,
        QDockWidget#TerminalDock::float-button,
        QDockWidget#AlifOutputDock::close-button,
        QDockWidget#AlifOutputDock::float-button {
            background: transparent;
            border: none;
        }

        /* --- تصميم شريط القوائم --- */
        QMenuBar {
            font-family: "Tajawal", "Noto Kufi Arabic";
            background-color: #0f172a;
            color: #f1f5f9;
            padding: 3px;
            font-size: 14px;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 6px 12px;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: #1e293b;
            color: #3b82f6;
        }

        /* --- قوائم شريط القوائم --- */
        QMenu {
            font-family: "Tajawal", "Noto Kufi Arabic";
            background-color: #1e293b;
            border: 1px solid #334155;
            color: #f1f5f9;
            border-radius: 6px;
            padding: 4px;
        }
        QMenu::item {
            background-color: transparent;
            border-radius: 4px;
            padding: 6px 21px 6px 18px;
        }
        QMenu::item:selected {
            background-color: #3b82f6;
            color: #ffffff;
        }
        QMenu::separator {
            height: 1px;
            background: #334155;
            margin: 4px 8px;
        }

        /* --- تصميم شريط الأدوات --- */
        QToolBar {
            background-color: #0f172a;
            border: none;
            padding: 6px;
            spacing: 8px; /* مسافة بين كل زر والآخر */
        }

        /* تصميم أزرار شريط الأدوات */
        QToolBar QToolButton {
            background-color: transparent;
            color: #f1f5f9;
            border: none;
            border-radius: 6px;

            /*  أهم جزء: تحديد حجم مربع الزر ليكون كبيراً ومربعاً */
            min-width: 40px;
            max-width: 40px;
            min-height: 40px;
            max-height: 40px;
        }

        QToolBar QToolButton:hover {
            background-color: #1e293b;
        }

        QToolBar QToolButton:pressed {
            background-color: #334155;
        }

        QToolBar QToolButton:checked {
            background-color: #3b82f6; /* اللون الأزرق */
        }

        /* --- تصميم شجرة المسارات--- */
        QTreeView {
            background-color: #0f172a;
            border: none;
            color: #94a3b8;
            font-size: 14px;
            outline: none;
        }
        QTreeView::item {
            padding: 4px 2px;
        }
        QTreeView::item:hover {
            background-color: #1e293b;
            color: #f1f5f9;
        }
        QTreeView::item:selected {
            background-color: #1e293b;
            color: #3b82f6;
        }

        /* --- تصميم الفاصل --- */
        QSplitter::handle {
            background-color: #094771;
            width: 1px;
        }

        /* --- تصميم التبويبات --- */
        QTabWidget::pane {
            border: none;
            background-color: #0f172a;
        }
        QTabWidget QTabBar { /* شريط التبويبات */
            background-color: #0f172a;
            qproperty-drawBase: 0;
        }
       QTabWidget QTabBar::tab {
            background: #0f172a;
            color: #94a3b8;
            padding: 3px 9px;
            min-width: 100px;
        }
       QTabWidget QTabBar::tab:selected {
            background: #0f172a;
            color: #3b82f6;
            border-bottom: 2px solid #3b82f6;
        }
        QTabWidget QTabBar::tab:hover:!selected {
            background: #334466;
            color: #f1f5f9;
        }
        QTabWidget QTabBar::close-button {
            image: url(:/icons/resources/close.svg);
            background: transparent;
            border-radius: 2px;
            padding: 1px;
            margin: 0px;
        }
        QTabWidget QTabBar::close-button:hover {
            background: #ef4444;
        }

        /* --- Status Bar --- */
        QStatusBar {
            background-color: #0f172a;
            color: #64748b;
            border-top: 1px solid #1e293b;
            font-size: 11px;
            padding-left: 10px;
        }
    )";
    setStyleSheet(styleSheet);
}

void Taif::closeEvent(QCloseEvent* const event)
{
    if (recoveryCloseFlushPending) {
        event->ignore();
        return;
    }

    if (!recoveryCloseFlushAcknowledged) {
        for (int index = 0; index < tabWidget->count(); ++index) {
            if (auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index))) {
                if (!prepareEditorForClose(editor)) {
                    emit closeRejected();
                    event->ignore();
                    return;
                }
            }
        }

        if (recoveryCoordinator != nullptr) {
            recoveryCloseFlushPending = true;
            // Keep the document immutable until the flush result is known; an
            // edit made during the close grace period would otherwise race the
            // acknowledged final snapshot.
            setEnabled(false);
            flushRecoverySnapshots();
            event->ignore();
            return;
        }
    }

    event->accept();
}

bool Taif::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_F6) {
            toggleConsole();
            return true;
        }
    }
    return QMainWindow::eventFilter(object, event);
}

void Taif::goToLine()
{
    TEditor *editor = currentEditor();
    if (!editor) return;

    bool ok;
    // أقصى رقم هو عدد أسطر الملف الحالي
    int maxLine = editor->blockCount();

    int lineNumber = QInputDialog::getInt(this, "الذهاب إلى سطر",
                                          QString("أدخل رقم السطر (1 - %1):").arg(maxLine),
                                          1, 1, maxLine, 1, &ok);

    if (ok) {
        QTextBlock block = editor->document()->findBlockByNumber(lineNumber - 1);

        if (block.isValid()) {
            QTextCursor cursor = editor->textCursor();

            // تحريك المؤشر الى الكتلة المختارة
            cursor.setPosition(block.position());

            // تحديث المحرر
            editor->setTextCursor(cursor);
            editor->centerCursor();
            editor->setFocus();
        }
    }
}

namespace {

SearchReplaceEngine::Query searchQuery(const SearchPanel* const panel)
{
    return {
        panel->searchText(),
        panel->replaceText(),
        panel->isCaseSensitive(),
        panel->isWholeWord(),
        panel->isRegex()
    };
}

void applySearchHighlights(TEditor* const editor,
                           const QList<QPair<int, int>>& matches,
                           const int currentIndex)
{
    if (editor == nullptr) {
        return;
    }

    QList<QTextEdit::ExtraSelection> selections;
    const QColor matchColor(245, 158, 11, 70);
    const QColor currentMatchColor(245, 158, 11, 160);
    for (int index = 0; index < matches.size(); ++index) {
        QTextEdit::ExtraSelection selection;
        QTextCursor cursor(editor->document());
        cursor.setPosition(matches.at(index).first);
        cursor.setPosition(matches.at(index).first + matches.at(index).second,
                           QTextCursor::KeepAnchor);
        selection.cursor = cursor;
        selection.format.setBackground(index == currentIndex ? currentMatchColor : matchColor);
        selections.append(selection);
    }
    editor->setExtraSelections(selections);
}

} // namespace

void Taif::showFindBar()
{
    if (currentEditor() == nullptr) {
        return;
    }
    searchBar->showReplaceRow(false);
    searchBar->showIn(currentEditor());
}

void Taif::showReplaceBar()
{
    if (currentEditor() == nullptr) {
        return;
    }
    searchBar->showReplaceRow(true);
    searchBar->showIn(currentEditor());
}

void Taif::clearSearchHighlights()
{
    if (TEditor* const editor = currentEditor()) {
        editor->setExtraSelections({});
    }
}

void Taif::hideFindBar()
{
    searchBar->hide();
    clearSearchHighlights();
    if (TEditor* const editor = currentEditor()) {
        editor->setFocus(Qt::OtherFocusReason);
    }
}

void Taif::performSearch(const bool forward, const bool next)
{
    TEditor* const editor = currentEditor();
    const QString pattern = searchBar->searchText();
    if (editor == nullptr || pattern.isEmpty()) {
        clearSearchHighlights();
        searchBar->setMatchInfo(0, 0);
        searchBar->setNoMatchesFound(false);
        return;
    }

    const SearchReplaceEngine::Query query = searchQuery(searchBar);
    if (!SearchReplaceEngine::isValid(query)) {
        clearSearchHighlights();
        searchBar->setMatchInfo(0, 0);
        searchBar->setNoMatchesFound(true);
        return;
    }

    const QList<QPair<int, int>> matches = SearchReplaceEngine::collectMatches(
        editor->toPlainText(), query);
    if (matches.isEmpty()) {
        clearSearchHighlights();
        searchBar->setMatchInfo(0, 0);
        searchBar->setNoMatchesFound(true);
        return;
    }

    const QTextCursor cursor = editor->textCursor();
    const int selectionStart = cursor.selectionStart();
    const int selectionEnd = cursor.selectionEnd();
    int currentIndex = 0;

    if (!next) {
        bool selectedMatchFound = false;
        for (int index = 0; index < matches.size(); ++index) {
            if (matches.at(index).first == selectionStart
                && matches.at(index).first + matches.at(index).second == selectionEnd) {
                currentIndex = index;
                selectedMatchFound = true;
                break;
            }
        }
        if (!selectedMatchFound) {
            for (int index = 0; index < matches.size(); ++index) {
                if (matches.at(index).first >= selectionStart) {
                    currentIndex = index;
                    break;
                }
                currentIndex = index;
            }
        }
    } else if (forward) {
        bool foundNext = false;
        for (int index = 0; index < matches.size(); ++index) {
            if (matches.at(index).first >= selectionEnd) {
                currentIndex = index;
                foundNext = true;
                break;
            }
        }
        if (!foundNext) {
            currentIndex = 0;
        }
    } else {
        currentIndex = matches.size() - 1;
        for (int index = matches.size() - 1; index >= 0; --index) {
            if (matches.at(index).first < selectionStart) {
                currentIndex = index;
                break;
            }
        }
    }

    applySearchHighlights(editor, matches, currentIndex);
    QTextCursor matchCursor(editor->document());
    matchCursor.setPosition(matches.at(currentIndex).first);
    matchCursor.setPosition(matches.at(currentIndex).first + matches.at(currentIndex).second,
                            QTextCursor::KeepAnchor);
    editor->setTextCursor(matchCursor);
    editor->ensureCursorVisible();
    searchBar->setMatchInfo(currentIndex + 1, matches.size());
    searchBar->setNoMatchesFound(false);
}

void Taif::findText() { performSearch(true, false); }
void Taif::findNextText() { performSearch(true, true); }
void Taif::findPrevText() { performSearch(false, true); }

void Taif::replaceOne()
{
    TEditor* const editor = currentEditor();
    const QString pattern = searchBar->searchText();
    if (editor == nullptr || pattern.isEmpty() || editor->isReadOnly()) {
        return;
    }

    const SearchReplaceEngine::Query query = searchQuery(searchBar);
    if (!SearchReplaceEngine::isValid(query)) {
        searchBar->setNoMatchesFound(true);
        return;
    }

    const QString text = editor->toPlainText();
    const QList<QPair<int, int>> matches = SearchReplaceEngine::collectMatches(text, query);
    if (matches.isEmpty()) {
        searchBar->setNoMatchesFound(true);
        return;
    }

    const int selectionStart = editor->textCursor().selectionStart();
    int targetIndex = 0;
    for (int index = 0; index < matches.size(); ++index) {
        const int matchStart = matches.at(index).first;
        const int matchEnd = matchStart + matches.at(index).second;
        if (matchStart <= selectionStart && selectionStart < matchEnd) {
            targetIndex = index;
            break;
        }
    }

    QTextCursor editCursor(editor->document());
    editCursor.setPosition(matches.at(targetIndex).first);
    editCursor.setPosition(matches.at(targetIndex).first + matches.at(targetIndex).second,
                           QTextCursor::KeepAnchor);
    editCursor.beginEditBlock();
    editCursor.insertText(SearchReplaceEngine::replacementForMatch(
        text.mid(matches.at(targetIndex).first, matches.at(targetIndex).second), query));
    editCursor.endEditBlock();
    editor->setTextCursor(editCursor);
    performSearch(true, true);
}

void Taif::replaceAll()
{
    TEditor* const editor = currentEditor();
    const QString pattern = searchBar->searchText();
    if (editor == nullptr || pattern.isEmpty() || editor->isReadOnly()) {
        return;
    }

    const SearchReplaceEngine::Query query = searchQuery(searchBar);
    if (!SearchReplaceEngine::isValid(query)) {
        searchBar->setNoMatchesFound(true);
        return;
    }

    const QString originalText = editor->toPlainText();
    const QList<QPair<int, int>> matches = SearchReplaceEngine::collectMatches(originalText, query);
    if (matches.isEmpty()) {
        searchBar->setNoMatchesFound(true);
        return;
    }

    SearchReplaceEngine::replaceAll(editor->document(), originalText, matches, query);

    performSearch(true, false);
}


void Taif::syncBottomToolActionState()
{
    if (menuBar == nullptr) {
        return;
    }

    menuBar->setOpenViewToolActions(
        alifOutputDock != nullptr && alifOutputDock->isVisible(),
        terminalDock != nullptr && terminalDock->isVisible(),
        diagnosticsDock != nullptr && diagnosticsDock->isVisible());
}

void Taif::showAndRaiseDock(QDockWidget* const dock)
{
    if (dock == nullptr) {
        return;
    }

    if (dock == terminalDock || dock == alifOutputDock) {
        DockableConsoleToolFactory::ensureTabifiedWith(this, diagnosticsDock, dock);
    }
    if (dock == terminalDock && systemTerminal != nullptr) {
        QString workingDirectory = folderPath;
        if (!QDir(workingDirectory).exists()) {
            if (TEditor* const editor = currentEditor(); editor != nullptr && !editor->filePath.isEmpty()) {
                workingDirectory = QFileInfo(editor->filePath).absolutePath();
            }
        }
        systemTerminal->setTerminalWorkingDirectory(workingDirectory);
    }
    DockableConsoleToolFactory::showAndActivate(dock);
    QTimer::singleShot(0, this, &Taif::syncBottomToolActionState);
}

void Taif::toggleConsole()
{
    if (terminalDock == nullptr) {
        return;
    }
    if (DockableConsoleToolFactory::isRenderedTab(terminalDock)) {
        terminalDock->hide();
        syncBottomToolActionState();
        if (TEditor* editor = currentEditor()) {
            editor->setFocus();
        }
        return;
    }
    showAndRaiseDock(terminalDock);
}

/* ----------------------------------- File Menu Button ----------------------------------- */

Taif::SaveDecision Taif::requestSaveDecision(TEditor* const editor) const
{
    if (editor == nullptr || !editor->document()->isModified()) {
        return SaveDecision::Discard;
    }

    QMessageBox messageBox;
    messageBox.setWindowTitle(QStringLiteral("طيف"));
    messageBox.setText(QStringLiteral("تم التعديل على الملف.\nهل تريد حفظ التغييرات؟"));
    auto* const saveButton = messageBox.addButton(QStringLiteral("حفظ"), QMessageBox::AcceptRole);
    auto* const discardButton = messageBox.addButton(QStringLiteral("تجاهل"), QMessageBox::DestructiveRole);
    auto* const cancelButton = messageBox.addButton(QStringLiteral("إلغاء"), QMessageBox::RejectRole);
    messageBox.setDefaultButton(cancelButton);

    QFont messageFont = font();
    messageFont.setPointSize(10);
    saveButton->setFont(messageFont);
    discardButton->setFont(messageFont);
    cancelButton->setFont(messageFont);
    messageBox.exec();

    if (messageBox.clickedButton() == saveButton) {
        return SaveDecision::Save;
    }
    if (messageBox.clickedButton() == discardButton) {
        return SaveDecision::Discard;
    }
    return SaveDecision::Cancel;
}

bool Taif::prepareEditorForClose(TEditor* const editor)
{
    switch (requestSaveDecision(editor)) {
    case SaveDecision::Save:
        return saveEditor(editor);
    case SaveDecision::Discard:
        editor->removeBackupFile();
        return true;
    case SaveDecision::Cancel:
        return false;
    }
    return false;
}

void Taif::newFile() {

    if (TEditor* const editor = currentEditor()) {
        if (!prepareEditorForClose(editor)) {
            return;
        }
    }

        TEditor *newEditor = new TEditor(setting, this);
    registerEditorRecovery(newEditor);
    tabWidget->addTab(newEditor, "غير معنون");

    tabWidget->setCurrentWidget(newEditor);

    connect(newEditor, &TEditor::openRequest, this, [this](QString filePath){this->openFile(filePath);});
    connectEditorDiagnostics(newEditor);
    connectEditorActionState(newEditor);
        connect(newEditor->document(), &QTextDocument::modificationChanged, this,
            [this, newEditor](const bool modified) {
                onEditorModificationChanged(newEditor, modified);
            });
    updateWindowTitle();

}

void Taif::openFile(QString filePath)
{
    if (TEditor* const current = currentEditor()) {
        if (!prepareEditorForClose(current)) {
            return;
        }
    }

    if (filePath.isEmpty()) {
        filePath = QFileDialog::getOpenFileName(
            this, QStringLiteral("فتح ملف"), {},
            QStringLiteral("ملف ألف (*.alif *.aliflib);;كل الملفات (*)"));
    }
    if (filePath.isEmpty()) {
        return;
    }

    QString failureMessage;
    if (!openDocumentFile(filePath, true, true, true, &failureMessage)) {
        QMessageBox::warning(this, QStringLiteral("خطأ"),
                             failureMessage.isEmpty()
                                 ? QStringLiteral("لا يمكن فتح الملف.")
                                 : failureMessage);
    }
}

bool Taif::openDocumentFile(const QString& requestedPath,
                            const bool promptForBackupRecovery,
                            const bool activateTab,
                            const bool updateRecentFiles,
                            QString* const failureMessage)
{
    const QString filePath = SessionStore::normalizePath(requestedPath);
    if (filePath.isEmpty()) {
        if (failureMessage != nullptr) {
            *failureMessage = QStringLiteral("مسار الملف غير صالح.");
        }
        return false;
    }

    for (int index = 0; index < tabWidget->count(); ++index) {
        auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index));
        if (editor != nullptr
            && SessionStore::normalizePath(editor->property("filePath").toString()) == filePath) {
            if (activateTab) {
                tabWidget->setCurrentIndex(index);
            }
            return true;
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (failureMessage != nullptr) {
            *failureMessage = QStringLiteral("لا يمكن فتح الملف: %1").arg(filePath);
        }
        return false;
    }
    QTextStream stream(&file);
    const QString content = stream.readAll();
    file.close();

    auto* const newEditor = new TEditor(setting, this);
    registerEditorRecovery(newEditor);
    newEditor->setPlainText(content);
    newEditor->filePath = filePath;
    newEditor->setProperty("filePath", filePath);
#if defined(Q_OS_WIN)
    newEditor->setDocumentLineEnding(EditorInfoSnapshot::LineEnding::Crlf);
#else
    newEditor->setDocumentLineEnding(EditorInfoSnapshot::LineEnding::Lf);
#endif
    newEditor->document()->setModified(false);
    newEditor->removeBackupFile();

    const QString backupPath = filePath + QStringLiteral(".~");
    if (promptForBackupRecovery && QFile::exists(backupPath)) {
        const QMessageBox::StandardButton reply = QMessageBox::warning(
            this, QStringLiteral("استعادة ملف"),
            QStringLiteral("يبدو أن البرنامج أُغلق بشكل غير متوقع.\n"
                           "يوجد نسخة محفوظة تلقائيًا أحدث من الملف الأصلي.\n\n"
                           "هل تريد استعادتها؟"),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QFile backup(backupPath);
            if (backup.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream backupStream(&backup);
                newEditor->setPlainText(backupStream.readAll());
                newEditor->document()->setModified(true);
            }
        } else {
            QFile::remove(backupPath);
        }
    }

    connect(newEditor, &TEditor::openRequest, this,
            [this](const QString& path) { openFile(path); });
    connect(newEditor->document(), &QTextDocument::modificationChanged, this,
            [this, newEditor](const bool modified) {
                onEditorModificationChanged(newEditor, modified);
            });
    connectEditorDiagnostics(newEditor);
    connectEditorActionState(newEditor);

    const QFileInfo fileInfo(filePath);
    const int tabIndex = tabWidget->addTab(newEditor, fileInfo.fileName());
    tabWidget->setTabToolTip(tabIndex, filePath);
    if (activateTab) {
        tabWidget->setCurrentIndex(tabIndex);
    }

    if (updateRecentFiles) {
        QSettings settings(QStringLiteral("Alif"), QStringLiteral("Taif"));
        QStringList recentFiles = settings.value(QStringLiteral("RecentFiles")).toStringList();
        const int recentFilesLimit = PreferencesStore::load().recentFilesLimit;
        recentFiles.removeAll(filePath);
        if (recentFilesLimit > 0) {
            recentFiles.prepend(filePath);
            while (recentFiles.size() > recentFilesLimit) {
                recentFiles.removeLast();
            }
        } else {
            recentFiles.clear();
        }
        settings.setValue(QStringLiteral("RecentFiles"), recentFiles);
    }

    updateWindowTitle();
    if (activateTab) {
        refreshBreadcrumbs();
    }
    return true;
}

SessionRestoreResult Taif::restoreSession(const SavedSession& savedSession)
{
    const SavedSession session = SessionStore::normalize(savedSession);
    SessionRestoreResult result;
    QSet<QString> openedPathKeys;

    for (const QString& path : session.filePaths) {
        const QString normalizedPath = SessionStore::normalizePath(path);
        const QString pathKey = normalizedPath.toCaseFolded();
        if (normalizedPath.isEmpty() || openedPathKeys.contains(pathKey)) {
            continue;
        }
        openedPathKeys.insert(pathKey);

        QString failureMessage;
        if (openDocumentFile(normalizedPath, false, false, true, &failureMessage)) {
            result.openedFilePaths.append(normalizedPath);
        } else {
            result.unavailableFilePaths.append(normalizedPath);
        }
    }

    const QString activePath = SessionStore::normalizePath(session.activeFilePath);
    int activeIndex = -1;
    for (int index = 0; index < tabWidget->count(); ++index) {
        auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index));
        if (editor != nullptr
            && SessionStore::normalizePath(editor->property("filePath").toString()) == activePath) {
            activeIndex = index;
            break;
        }
    }
    if (activeIndex >= 0) {
        tabWidget->setCurrentIndex(activeIndex);
    } else if (tabWidget->count() > 0) {
        tabWidget->setCurrentIndex(0);
    } else {
        newFile();
    }

    refreshDiagnosticsPanel();
    updateEditActionState();
    refreshBreadcrumbs();
    return result;
}

void Taif::loadFolder(const QString& requestedFolderPath)
{
    const QString normalizedFolderPath = SessionStore::normalizePath(requestedFolderPath);
    if (!normalizedFolderPath.isEmpty() && QDir(normalizedFolderPath).exists()) {
        folderPath = normalizedFolderPath;
        projectExplorer->setProjectRoot(folderPath);
        gitPanel->setProjectRoot(folderPath);
        projectExplorer->setVisible(true);
        toggleSidebarAction->setChecked(true);
    } else {
        folderPath.clear();
        projectExplorer->setProjectRoot({});
        gitPanel->setProjectRoot({});
        projectExplorer->setVisible(false);
        toggleSidebarAction->setChecked(false);
    }
    refreshBreadcrumbs();
}

void Taif::handleOpenFolderMenu()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "اختر مجلد", QDir::homePath());
    if (folderPath.isEmpty()) return;

    loadFolder(folderPath);

}

void Taif::toggleSidebar()
{
    const bool shouldBeVisible = !projectExplorer->isVisible();
    projectExplorer->setVisible(shouldBeVisible);
    toggleSidebarAction->setChecked(shouldBeVisible);
    if (shouldBeVisible) {
        projectExplorer->setFocus();
    }
}

void Taif::onProjectFileActivated(const QString& filePath)
{
    QString failureMessage;
    if (!openDocumentFile(filePath, true, true, true, &failureMessage)) {
        QMessageBox::warning(this, QStringLiteral("خطأ"),
                             failureMessage.isEmpty() ? QStringLiteral("لا يمكن فتح الملف.")
                                                      : failureMessage);
    }
}

void Taif::createProjectFile(const QString& directoryPath, const QString& name)
{
    const ProjectFileOperationResult result = ProjectFileOperations::createFile(folderPath, directoryPath, name);
    presentProjectOperationResult(result);
    if (result.succeeded) {
        projectExplorer->selectPath(result.destinationPath);
        onProjectFileActivated(result.destinationPath);
    }
}

void Taif::createProjectFolder(const QString& directoryPath, const QString& name)
{
    const ProjectFileOperationResult result = ProjectFileOperations::createFolder(folderPath, directoryPath, name);
    presentProjectOperationResult(result);
    if (result.succeeded) {
        projectExplorer->selectPath(result.destinationPath);
    }
}

void Taif::renameProjectPath(const QString& sourcePath, const QString& newName)
{
    if (hasOpenEditorAtOrBelow(sourcePath)) {
        const auto reply = QMessageBox::question(this, QStringLiteral("إعادة تسمية عنصر مفتوح"),
            QStringLiteral("يوجد ملف مفتوح داخل هذا المسار. سيُحدَّث مساره في المحرر بعد نجاح إعادة التسمية. هل تريد المتابعة؟"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }
    const ProjectFileOperationResult result = ProjectFileOperations::renamePath(folderPath, sourcePath, newName);
    presentProjectOperationResult(result);
    if (!result.succeeded) {
        return;
    }
    const QString oldPrefix = ProjectFileOperations::normalizedPath(sourcePath);
    const QString newPrefix = ProjectFileOperations::normalizedPath(result.destinationPath);
    for (int index = 0; index < tabWidget->count(); ++index) {
        auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index));
        if (editor == nullptr) continue;
        const QString editorPath = ProjectFileOperations::normalizedPath(editor->property("filePath").toString());
        if (editorPath == oldPrefix || editorPath.startsWith(oldPrefix + QLatin1Char('/'), Qt::CaseInsensitive)) {
            const QString suffix = editorPath.mid(oldPrefix.size());
            const QString updatedPath = newPrefix + suffix;
            editor->filePath = updatedPath;
            editor->setProperty("filePath", updatedPath);
            tabWidget->setTabText(index, QFileInfo(updatedPath).fileName());
            tabWidget->setTabToolTip(index, updatedPath);
        }
    }
    projectExplorer->selectPath(result.destinationPath);
    projectExplorer->refresh();
    refreshBreadcrumbs();
}

void Taif::deleteProjectPath(const QString& sourcePath)
{
    const QString normalizedSource = ProjectFileOperations::normalizedPath(sourcePath);
    for (int index = tabWidget->count() - 1; index >= 0; --index) {
        auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index));
        if (editor == nullptr) continue;
        const QString editorPath = ProjectFileOperations::normalizedPath(editor->property("filePath").toString());
        if (editorPath == normalizedSource || editorPath.startsWith(normalizedSource + QLatin1Char('/'), Qt::CaseInsensitive)) {
            if (!prepareEditorForClose(editor)) {
                return;
            }
        }
    }
    const ProjectFileOperationResult result = ProjectFileOperations::moveToTrash(folderPath, sourcePath);
    presentProjectOperationResult(result);
    if (!result.succeeded) {
        const auto fallback = QMessageBox::warning(this, QStringLiteral("تعذر النقل إلى سلة المحذوفات"),
            QStringLiteral("هل تريد الحذف نهائياً؟ لا يمكن التراجع عن هذا الإجراء."),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (fallback != QMessageBox::Yes) return;
        const ProjectFileOperationResult permanentResult = ProjectFileOperations::permanentlyDelete(folderPath, sourcePath);
        presentProjectOperationResult(permanentResult);
        if (!permanentResult.succeeded) return;
    }
    for (int index = tabWidget->count() - 1; index >= 0; --index) {
        auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index));
        if (editor == nullptr) continue;
        const QString editorPath = ProjectFileOperations::normalizedPath(editor->property("filePath").toString());
        if (editorPath == normalizedSource || editorPath.startsWith(normalizedSource + QLatin1Char('/'), Qt::CaseInsensitive)) {
            tabWidget->removeTab(index);
            editor->deleteLater();
        }
    }
    projectExplorer->refresh();
    refreshBreadcrumbs();
}

void Taif::revealProjectPath(const QString& sourcePath)
{
    presentProjectOperationResult(ProjectFileOperations::reveal(folderPath, sourcePath));
}

void Taif::showGitPanel()
{
    if (gitPanel->projectRoot() != folderPath) {
        gitPanel->setProjectRoot(folderPath);
    }
    gitDock->show();
    gitDock->raise();
    gitPanel->refresh();
}

void Taif::handleGitDestructiveOperation(const GitOperation operation, const QStringList& relativePaths)
{
    if (operation != GitOperation::Discard || folderPath.isEmpty() || relativePaths.isEmpty()) {
        return;
    }
    for (const QString& relativePath : relativePaths) {
        if (hasOpenEditorAtOrBelow(QDir(folderPath).filePath(relativePath))) {
            QMessageBox::warning(this, QStringLiteral("حماية المحرر"),
                QStringLiteral("أغلق أو احفظ الملف المفتوح قبل تجاهل تعديلاته من Git."));
            return;
        }
    }
    const auto reply = QMessageBox::warning(this, QStringLiteral("تجاهل تعديلات Git"),
        QStringLiteral("سيجري فقدان التعديلات المحلية في الملفات المحددة نهائياً. هل تريد المتابعة؟"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
        projectExplorer->gitRepositoryService()->discard(relativePaths);
    }
}

void Taif::handleGitPull()
{
    const GitRepositorySnapshot& snapshot = projectExplorer->gitRepositoryService()->snapshot();
    if (!snapshot.repository) return;
    for (int index = 0; index < tabWidget->count(); ++index) {
        auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index));
        if (editor != nullptr && editor->document()->isModified()) {
            QMessageBox::warning(this, QStringLiteral("حماية المحرر"),
                QStringLiteral("احفظ أو أغلق الملفات المعدلة قبل السحب من Git."));
            return;
        }
    }
    if (snapshot.isDirty()) {
        const auto reply = QMessageBox::question(this, QStringLiteral("سحب Git"),
            QStringLiteral("توجد تغييرات محلية. سيُنفذ السحب السريع فقط. هل تريد المتابعة؟"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (reply != QMessageBox::Yes) return;
    }
    projectExplorer->gitRepositoryService()->pull();
}

void Taif::handleGitBranchSwitch(const QString& branch)
{
    const GitRepositorySnapshot& snapshot = projectExplorer->gitRepositoryService()->snapshot();
    if (!snapshot.repository) return;
    for (int index = 0; index < tabWidget->count(); ++index) {
        auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index));
        if (editor != nullptr && editor->document()->isModified()) {
            QMessageBox::warning(this, QStringLiteral("حماية المحرر"),
                QStringLiteral("احفظ أو أغلق الملفات المعدلة قبل تبديل فرع Git."));
            return;
        }
    }
    if (snapshot.isDirty()) {
        const auto reply = QMessageBox::question(this, QStringLiteral("تبديل فرع Git"),
            QStringLiteral("توجد تغييرات محلية. هل تريد متابعة تبديل الفرع؟"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (reply != QMessageBox::Yes) return;
    }
    projectExplorer->gitRepositoryService()->switchBranch(branch);
}

void Taif::saveFile()
{
    [[maybe_unused]] const bool saved = saveEditor(currentEditor());
}

void Taif::saveFileAs()
{
    [[maybe_unused]] const bool saved = saveEditorAs(currentEditor());
}

bool Taif::saveEditor(TEditor* const editor)
{
    if (editor == nullptr) {
        return false;
    }

    const QString filePath = SessionStore::normalizePath(editor->property("filePath").toString());
    return filePath.isEmpty() ? saveEditorAs(editor) : writeEditorContents(editor, filePath);
}

bool Taif::saveEditorAs(TEditor* const editor)
{
    if (editor == nullptr) {
        return false;
    }

    const QString currentPath = editor->property("filePath").toString();
    const QString currentName = currentPath.isEmpty()
        ? QStringLiteral("ملف جديد.alif")
        : QFileInfo(currentPath).fileName();
    const QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("حفظ الملف"), currentName,
        QStringLiteral("ملف ألف (*.alif);;مكتبة ألف (*.aliflib);;كل الملفات (*)"));
    if (filePath.isEmpty()) {
        return false;
    }
    return writeEditorContents(editor, filePath);
}

bool Taif::writeEditorContents(TEditor* const editor, const QString& requestedPath)
{
    if (editor == nullptr) {
        return false;
    }

    const QString filePath = SessionStore::normalizePath(requestedPath);
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("خطأ"), QStringLiteral("مسار الحفظ غير صالح."));
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("خطأ"), QStringLiteral("لا يمكن حفظ الملف."));
        return false;
    }

    QTextStream output(&file);
    output << editor->toPlainText();
    output.flush();
    if (output.status() != QTextStream::Ok || !file.commit()) {
        QMessageBox::warning(this, QStringLiteral("خطأ"), QStringLiteral("تعذر إتمام حفظ الملف بأمان."));
        return false;
    }

    finalizeSavedEditor(editor, filePath);
    return true;
}

void Taif::registerEditorRecovery(TEditor* const editor)
{
    if (editor != nullptr && recoveryCoordinator != nullptr) {
        editor->setRecoveryCoordinator(recoveryCoordinator);
    }
}

void Taif::flushRecoverySnapshots()
{
    if (recoveryCoordinator == nullptr) {
        return;
    }
    for (int index = 0; index < tabWidget->count(); ++index) {
        if (auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index))) {
            editor->flushRecoverySnapshot();
        }
    }
    recoveryCoordinator->requestFlush(1200);
}

void Taif::importKnownLegacyRecoveryEntries(const QString& launchFilePath)
{
    if (recoveryCoordinator == nullptr) {
        return;
    }

    QSet<QString> candidatePaths;
    const auto addCandidate = [&candidatePaths](const QString& path) {
        const QString normalizedPath = SessionStore::normalizePath(path);
        if (!normalizedPath.isEmpty()) {
            candidatePaths.insert(normalizedPath);
        }
    };
    addCandidate(launchFilePath);

    QSettings settings(QStringLiteral("Alif"), QStringLiteral("Taif"));
    for (const QString& path : settings.value(QStringLiteral("RecentFiles")).toStringList()) {
        addCandidate(path);
    }
    const SessionStore sessionStore;
    for (const SavedSession& session : sessionStore.loadAll()) {
        for (const QString& path : session.filePaths) {
            addCandidate(path);
        }
    }

    for (const QString& path : candidatePaths) {
        [[maybe_unused]] const bool imported = recoveryCoordinator->importLegacyAdjacentBackup(path);
    }
}

void Taif::presentRecoveryEntries()
{
    if (recoveryCoordinator == nullptr) {
        return;
    }
    const QVector<RecoveryEntry> availableEntries = recoveryCoordinator->entries();
    if (availableEntries.isEmpty()) {
        return;
    }

    TRecoveryDialog dialog(availableEntries, this);
    dialog.exec();
    const QVector<RecoveryEntry> selectedEntries = dialog.selectedEntries();
    if (dialog.decision() == TRecoveryDialog::Decision::Discard) {
        for (const RecoveryEntry& entry : selectedEntries) {
            recoveryCoordinator->removeEntry(entry.id);
        }
        recoveryCoordinator->requestFlush(1000);
        return;
    }
    if (dialog.decision() == TRecoveryDialog::Decision::Restore) {
        for (const RecoveryEntry& entry : selectedEntries) {
            restoreRecoveryEntry(entry);
        }
    }
}

void Taif::restoreRecoveryEntry(const RecoveryEntry& entry)
{
    if (recoveryCoordinator == nullptr) {
        return;
    }
    QString recoveredText;
    QString errorMessage;
    if (!recoveryCoordinator->readSnapshot(entry, &recoveredText, &errorMessage)) {
        QMessageBox::warning(this, QStringLiteral("خطأ"),
                             errorMessage.isEmpty() ? QStringLiteral("تعذر استعادة النسخة.")
                                                    : errorMessage);
        return;
    }

    const RecoverySourceFingerprint currentFingerprint =
        RecoveryStore::fingerprintForPath(entry.sourcePath);
    const bool sourceUnchanged = !entry.sourcePath.isEmpty()
        && currentFingerprint.exists == entry.sourceFingerprint.exists
        && currentFingerprint.size == entry.sourceFingerprint.size
        && currentFingerprint.lastModifiedUtc == entry.sourceFingerprint.lastModifiedUtc;

    auto* const editor = new TEditor(setting, this);
    registerEditorRecovery(editor);
    editor->adoptRecoveryEntry(entry);
    editor->setPlainText(recoveredText);
    if (sourceUnchanged) {
        editor->filePath = entry.sourcePath;
        editor->setProperty("filePath", entry.sourcePath);
    }
    editor->document()->setModified(true);

    connect(editor, &TEditor::openRequest, this,
            [this](const QString& path) { openFile(path); });
    connect(editor->document(), &QTextDocument::modificationChanged, this,
            [this, editor](const bool modified) { onEditorModificationChanged(editor, modified); });
    connectEditorDiagnostics(editor);
    connectEditorActionState(editor);

    const QString displayName = entry.displayName.isEmpty() ? QStringLiteral("غير معنون")
                                                             : entry.displayName;
    const int tabIndex = tabWidget->addTab(editor, QStringLiteral("استعادة — %1").arg(displayName));
    tabWidget->setTabToolTip(tabIndex, entry.sourcePath.isEmpty()
        ? QStringLiteral("نسخة استعادة لمستند غير معنون") : entry.sourcePath);
    tabWidget->setCurrentIndex(tabIndex);
    onEditorModificationChanged(editor, true);
}

void Taif::finalizeSavedEditor(TEditor* const editor, const QString& filePath)
{
    if (editor == nullptr) {
        return;
    }

    editor->filePath = filePath;
    editor->setProperty("filePath", filePath);
#if defined(Q_OS_WIN)
    editor->setDocumentLineEnding(EditorInfoSnapshot::LineEnding::Crlf);
#else
    editor->setDocumentLineEnding(EditorInfoSnapshot::LineEnding::Lf);
#endif
    editor->document()->setModified(false);
    editor->removeBackupFile();
    onEditorModificationChanged(editor, false);
    if (editor == currentEditor()) {
        refreshBreadcrumbs();
    }
}

void Taif::openSettings()
{
    if (setting == nullptr) {
        return;
    }
    if (!setting->isVisible()) {
        setting->beginEditing();
    }
    setting->show();
    setting->raise();
    setting->activateWindow();
}


void Taif::exitApp()
{
    emit returnToWelcomeRequested();
}

void Taif::onCurrentTabChanged()
{
    if (searchBar != nullptr && searchBar->isVisible()) {
        searchBar->hide();
    }
    updateWindowTitle();

    TEditor* const editor = currentEditor();
    bindInformationBarToEditor(editor);
    refreshEditorInfoBar();

    refreshDiagnosticsPanel();
    updateEditActionState();
    bindBreadcrumbsToEditor(editor);
    refreshBreadcrumbs();
}

void Taif::connectEditorActionState(TEditor* const editor)
{
    if (editor == nullptr) {
        return;
    }

    connect(editor, &QPlainTextEdit::copyAvailable, this,
            [this](const bool) { updateEditActionState(); });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this,
            [this]() { updateEditActionState(); });
    connect(editor, &QPlainTextEdit::undoAvailable, this,
            [this](const bool) { updateEditActionState(); });
    connect(editor, &QPlainTextEdit::redoAvailable, this,
            [this](const bool) { updateEditActionState(); });
    connect(editor, &QPlainTextEdit::textChanged, this,
            [this]() { updateEditActionState(); });
}

void Taif::updateEditActionState()
{
    if (menuBar == nullptr) {
        return;
    }

    TEditor* const editor = currentEditor();
    const bool hasEditor = editor != nullptr;
    const bool editable = hasEditor && !editor->isReadOnly();
    const bool hasSelection = hasEditor && editor->textCursor().hasSelection();
    const QMimeData* const mimeData = QApplication::clipboard()->mimeData();
    const bool canPasteText = editable && mimeData != nullptr && mimeData->hasText();

    menuBar->undoAction->setEnabled(editable && editor->document()->isUndoAvailable());
    menuBar->redoAction->setEnabled(editable && editor->document()->isRedoAvailable());
    menuBar->cutAction->setEnabled(editable && hasSelection);
    menuBar->copyAction->setEnabled(hasSelection);
    menuBar->pasteAction->setEnabled(canPasteText);
    menuBar->findAction->setEnabled(hasEditor);
    menuBar->replaceAction->setEnabled(editable);
    menuBar->goToLineAction->setEnabled(hasEditor);
    menuBar->toggleCommentAction->setEnabled(editable);
    menuBar->duplicateLineAction->setEnabled(editable);
    menuBar->moveLineUpAction->setEnabled(editable);
    menuBar->moveLineDownAction->setEnabled(editable);
}

bool Taif::hasOpenEditorAtOrBelow(const QString& path) const
{
    const QString normalizedPath = ProjectFileOperations::normalizedPath(path);
    if (normalizedPath.isEmpty()) {
        return false;
    }
    for (int index = 0; index < tabWidget->count(); ++index) {
        const auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index));
        if (editor == nullptr) continue;
        const QString editorPath = ProjectFileOperations::normalizedPath(editor->property("filePath").toString());
        if (editorPath == normalizedPath
            || editorPath.startsWith(normalizedPath + QLatin1Char('/'), Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

void Taif::presentProjectOperationResult(const ProjectFileOperationResult& result)
{
    if (result.succeeded) {
        statusBar()->showMessage(result.userMessage, 3500);
        return;
    }
    QMessageBox::warning(this, QStringLiteral("عملية ملفات المشروع"),
                         result.userMessage.isEmpty() ? QStringLiteral("تعذرت العملية.")
                                                      : result.userMessage);
}

void Taif::connectEditorDiagnostics(TEditor* editor) {
    if (editor == nullptr) {
        return;
    }
    connect(editor, &TEditor::diagnosticsChanged, this,
            [this, editor](QVector<EditorDiagnostic> diagnostics, const quint64) {
                if (editor == currentEditor() && diagnosticsPanel) {
                    diagnosticsPanel->setDiagnostics(std::move(diagnostics));
                }
            });
}

void Taif::refreshDiagnosticsPanel() {
    if (!diagnosticsPanel) {
        return;
    }
    if (TEditor* editor = currentEditor()) {
        diagnosticsPanel->setDiagnostics(editor->currentDiagnostics());
    } else {
        diagnosticsPanel->clearDiagnostics();
    }
}

void Taif::bindInformationBarToEditor(TEditor* const editor)
{
    if (editorInformationConnection) {
        disconnect(editorInformationConnection);
    }
    if (editor == nullptr || editorInfoBar == nullptr) {
        return;
    }
    editorInformationConnection = connect(editor, &TEditor::editorInformationChanged, this,
                                          [this, editor](const EditorInfoSnapshot& snapshot) {
        if (editor == currentEditor() && editorInfoBar != nullptr) {
            editorInfoBar->setSnapshot(snapshot);
        }
    });
}

void Taif::refreshEditorInfoBar()
{
    if (editorInfoBar == nullptr) {
        return;
    }
    if (TEditor* const editor = currentEditor()) {
        editorInfoBar->setSnapshot(editor->informationSnapshot());
    } else {
        editorInfoBar->setSnapshot({});
    }
}

void Taif::bindBreadcrumbsToEditor(TEditor* const editor)
{
    if (breadcrumbConnection) {
        disconnect(breadcrumbConnection);
    }
    if (editor == nullptr || breadcrumbBar == nullptr) {
        return;
    }
    breadcrumbConnection = connect(editor, &TEditor::breadcrumbContextChanged, this,
                                   [this, editor](const EditorBreadcrumbContext& context) {
        if (editor == currentEditor() && breadcrumbBar != nullptr) {
            breadcrumbBar->setSemanticContext(context);
        }
    });
}

void Taif::refreshBreadcrumbs()
{
    if (breadcrumbBar == nullptr) {
        return;
    }
    TEditor* const editor = currentEditor();
    if (editor == nullptr) {
        breadcrumbBar->setFileContext({});
        breadcrumbBar->clearSemanticContext();
        return;
    }

    breadcrumbBar->setFileContext(
        SessionStore::normalizePath(editor->property("filePath").toString()));
    breadcrumbBar->setSemanticContext(editor->breadcrumbContextAtCursor());
}

void Taif::revealBreadcrumbPath(const QString& path)
{
    const QFileInfo pathInfo(path);
    if (!pathInfo.isDir()) {
        if (TEditor* const editor = currentEditor()) {
            editor->setFocus();
        }
        return;
    }

    if (folderPath.isEmpty()) {
        return;
    }
    const QString relativePath = QDir(folderPath).relativeFilePath(pathInfo.absoluteFilePath());
    if (relativePath == QStringLiteral("..")
        || relativePath.startsWith(QStringLiteral("../"))
        || relativePath.startsWith(QStringLiteral("..\\"))) {
        return;
    }
    projectExplorer->setVisible(true);
    toggleSidebarAction->setChecked(true);
    projectExplorer->selectPath(pathInfo.absoluteFilePath());
}

void Taif::updateCursorPosition()
{
    refreshEditorInfoBar();
}


/* ----------------------------------- Run Menu Button ----------------------------------- */

// void Taif::runAlif() {
//     QString program{};
//     QStringList args{};
//     QString command{};
//     TEditor *editor = currentEditor(); // ✅ احصل على المحرر النشط
//     QStringList arguments{editor->filePath};
//     QString workingDirectory = QCoreApplication::applicationDirPath();

//     if (editor->filePath.isEmpty() or (currentEditor() && currentEditor()->document()->isModified())) {
//         QMessageBox::warning(nullptr, "تنبيه", "قم بحفظ الملف لتشغيله");
//         return;
//     }

// #if defined(Q_OS_WINDOWS)
//     // Windows: Start cmd.exe with /K to keep the window open
//     program = "cmd.exe";
//     command = "alif\\alif.exe";
//     args << "/C" << "start" << program << "/K" << command << arguments;
// #elif defined(Q_OS_LINUX)
//     // Linux: Use x-terminal-emulator with -e to execute the command
//     program = "x-terminal-emulator";
//     command = "./alif/alif";
//     if (!arguments.isEmpty()) {
//         command += " " + arguments.join(" ");
//     }
//     command += "; exec bash";
//     args << "-e" << "bash" << "-c" << command;
// #elif defined(Q_OS_MACOS)
//     // macOS: Use AppleScript to run the command in Terminal.app
//     program = "osascript";
//     command = "./alif/alif";

//     // Escape each part for shell execution
//     QStringList allParts = QStringList() << command << arguments;
//     QStringList escapedShellParts;
//     for (const QString &part : allParts) {
//         QString escaped = part;
//         escaped.replace("'", "'\"'\"'"); // Escape single quotes for AppleScript
//         escapedShellParts << "'" + escaped + "'";
//     }
//     QString shellCommand = escapedShellParts.join(" ");

//     // Escape double quotes for AppleScript
//     QString escapedAppleScriptCommand = shellCommand.replace("\"", "\\\"");

//     // Construct AppleScript
//     QString script = QString(
//                          "tell application \"Terminal\"\n"
//                          "    activate\n"
//                          "    do script \"cd '%1' && %2\"\n"
//                          "end tell"
//                          ).arg(workingDirectory, escapedAppleScriptCommand);

//     args << "-e" << script;
// #endif

//     QProcess* process = new QProcess(this);
//     process->setWorkingDirectory(workingDirectory);

//     process->start(program, args);
// }

//----------------

void Taif::runAlif()
{
    TEditor* const editor = currentEditor();
    TConsole* const console = alifOutputConsole;
    if (editor == nullptr || console == nullptr || alifOutputDock == nullptr
        || runController == nullptr) {
        return;
    }

    showAndRaiseDock(alifOutputDock);
    if (runController->isActive()) {
        console->appendPlainTextThreadSafe(QStringLiteral("\nجار إيقاف التنفيذ...\n"));
        runController->cancel();
        return;
    }

    QString filePath = editor->property("filePath").toString();
    if (filePath.isEmpty() || editor->document()->isModified()) {
        QMessageBox::warning(this, QStringLiteral("تنبيه"),
                             QStringLiteral("يجب حفظ الملف قبل التشغيل."));
        saveFile();
        filePath = editor->property("filePath").toString();
        if (filePath.isEmpty() || editor->document()->isModified()) {
            return;
        }
    }

    const QString applicationDirectory = QCoreApplication::applicationDirPath();
#if defined(Q_OS_WIN)
    const QString program = QDir(applicationDirectory).filePath(QStringLiteral("alif/alif.exe"));
#else
    const QString program = QDir(applicationDirectory).filePath(QStringLiteral("alif/alif"));
#endif
    if (!QFileInfo::exists(program)) {
        console->clear();
        console->appendPlainTextThreadSafe(
            QStringLiteral("تعذر العثور على مترجم ألف.\nالمسار المتوقع: %1\n").arg(program));
#if defined(Q_OS_LINUX)
        console->appendPlainTextThreadSafe(
            QStringLiteral("تأكد من وجود ملف ألف ومن منحه صلاحية التنفيذ.\n"));
#endif
        return;
    }

    console->clear();
    console->appendPlainTextThreadSafe(
        QStringLiteral("بدء تشغيل ملف ألف: %1\n\n").arg(QFileInfo(filePath).fileName()));

    AlifRunController::Request request;
    request.program = program;
    request.arguments = {filePath};
    request.workingDirectory = QFileInfo(filePath).absolutePath();
    request.displayName = QFileInfo(filePath).fileName();

    QString errorMessage;
    if (!runController->start(request, &errorMessage)) {
        console->appendPlainTextThreadSafe(
            QStringLiteral("تعذر تشغيل ألف: %1\n").arg(errorMessage));
    }
}

//----------------

TEditor* Taif::currentEditor() {
    return qobject_cast<TEditor*>(tabWidget->currentWidget());
}

void Taif::closeTab(const int index)
{
    if (tabWidget->count() <= 1) {
        return;
    }

    auto* const editor = qobject_cast<TEditor*>(tabWidget->widget(index));
    if (editor == nullptr || !prepareEditorForClose(editor)) {
        return;
    }

    tabWidget->removeTab(index);
    editor->deleteLater();
}

/* ----------------------------------- Help Menu Button ----------------------------------- */

void Taif::aboutTaif() {
    QMessageBox messageDialog{};
    messageDialog.setWindowTitle("عن محرر طيف");
    messageDialog.setText(R"(
        محرر طيف (نـ3) 1445-1446

        © الحقوق محفوظة لصالح
        برمجيات ألف - عبدالرحمن ومحمد الخطيب - سوريا

        محرر نصي خاص بلغة ألف نـ5
        يعمل على جميع المنصات "ويندوز - لينكس - ماك"
        ـــــــــــــــــــــــــــــــــــــــــــــــــــــ
        المحرر لا يزال تحت التطوير وقد يحتوي بعض الاخطاء
        نرجو تبليغ مجتمع ألف في حال وجود أي خطأ
        https://t.me/aliflang
        ـــــــــــــــــــــــــــــــــــــــــــــــــــــ
        فريق التطوير لا يمتلك أي ضمانات وغير مسؤول
        عن أي خطأ او خلل قد يحدث بسبب المحرر.

        المحرر يخضع لرخصة برمجيات ألف
        يجب قراءة الرخصة جيداً قبل البدأ بإستخدام المحرر
                                    )"
                          );
    messageDialog.setStyleSheet("background: #1e293b; color: #f1f5f9; font-family: 'Tajawal', 'Noto Kufi Arabic';");

    messageDialog.exec();
}

void Taif::checkForUpdates() {
    #if defined(Q_OS_WIN)
    QString path = QCoreApplication::applicationDirPath() + "/MaintenanceTool.exe";
    #elif defined(Q_OS_LINUX)
    QString path = QCoreApplication::applicationDirPath() + "/maintenancetool";
    #elif defined(Q_OS_MACOS)
    QString path = QCoreApplication::applicationDirPath() + "/../../../maintenancetool.app/contents/MacOS/maintenancetool";
    qDebug() << path;
    #endif
    // Use --updater to skip the "Add/Remove" page and go straight to updates
    QProcess::startDetached(path, {"--su"});
}

/* ----------------------------------- Other Functions ----------------------------------- */

void Taif::updateWindowTitle() {
    TEditor* editor = currentEditor();
    QString title{};

    if (!editor) {
        title = "طيف";
    } else {
        QString filePath = editor->property("filePath").toString();
        // --------------------------------------------------------

        if (filePath.isEmpty()) {
            title = "غير معنون";
        } else {
            title = QFileInfo(filePath).fileName();
        }
        if (editor->document()->isModified()) {
            title += "[*]";
        }
        title += " - طيف";
    }
    setWindowTitle(title);
    setWindowModified(editor && editor->document()->isModified()); // تحديث علامة التعديل للنافذة
}

void Taif::onEditorModificationChanged(TEditor* const editor, const bool modified)
{
    if (editor == nullptr) {
        return;
    }

    const int index = tabWidget->indexOf(editor);
    if (index >= 0) {
        QString tabText = tabWidget->tabText(index);
        if (modified && !tabText.endsWith(QStringLiteral("[*]"))) {
            tabWidget->setTabText(index, tabText + QStringLiteral("[*]"));
        } else if (!modified && tabText.endsWith(QStringLiteral("[*]"))) {
            tabWidget->setTabText(index, tabText.left(tabText.length() - 3));
        }
    }

    if (editor == currentEditor()) {
        updateWindowTitle();
    }
}

