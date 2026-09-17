#include "Taif.h"
#include "TWelcomeWindow.h"
#include "TConsole.h"
#include "DockableConsoleTool.h"
#include "ProcessWorker.h"
#include "TSearchPanel.h"
#include "TDiagnosticsPanel.h"
#include "TBreadcrumbBar.h"

#include <QThread>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QShortcut>
#include <QGuiApplication>
#include <QScreen>
#include <QCoreApplication>
#include <QTextStream>
#include <QApplication>
#include <QToolBar>
#include <QHeaderView>
#include <QSettings>
#include <QProcess>
#include <QStyleFactory>
#include <QKeyEvent>
#include <QTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QSet>


Taif::Taif(const QString& filePath, QWidget* const parent,
           const bool createInitialDocument)
    : QMainWindow(parent)
{

    setAttribute(Qt::WA_DeleteOnClose);

    setting = new TSettings();

    setupUI();
    setupConnections();
    setupStyle();
    setupTimers();

    installEventFilter(this);

    if (!filePath.isEmpty()) {
        openFile(filePath);
    } else if (createInitialDocument) {
        newFile();
    }
}

Taif::~Taif() {
    if (thread) {
        thread->wait();
        thread->quit();
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
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    menuBar = new TMenuBar(this);
    setMenuBar(menuBar);

    mainSplitter = new QSplitter(Qt::Horizontal, this);
    fileTreeView = new QTreeView(this);
    fileSystemModel = new QFileSystemModel(this);

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

    QAction *runToolbarAction = new QAction(QIcon(":/icons/resources/run.svg"), "تشغيل الملف الحالي", this);

    mainToolBar->addAction(runToolbarAction);
    connect(runToolbarAction, &QAction::triggered, this, &Taif::runAlif);

    fileSystemModel->setRootPath(QDir::homePath());
    fileTreeView->setModel(fileSystemModel);
    fileTreeView->header()->setVisible(false);
    for(int i = 1; i <= 3; ++i) fileTreeView->hideColumn(i);
    fileTreeView->setRootIndex(fileSystemModel->index(QDir::homePath()));
    fileTreeView->setVisible(false);

    auto* const editorPane = new QWidget(editorSplitter);
    auto* const editorPaneLayout = new QVBoxLayout(editorPane);
    editorPaneLayout->setContentsMargins(0, 0, 0, 0);
    editorPaneLayout->setSpacing(0);
    breadcrumbBar = new TBreadcrumbBar(editorPane);
    editorPaneLayout->addWidget(breadcrumbBar);
    editorPaneLayout->addWidget(tabWidget, 1);
    editorSplitter->addWidget(editorPane);
    editorSplitter->addWidget(searchBar);
    editorSplitter->setSizes({1000, 45});

    mainSplitter->addWidget(fileTreeView);
    mainSplitter->addWidget(editorSplitter);
    mainSplitter->setSizes({200, 700});
    this->setCentralWidget(mainSplitter);

    diagnosticsDock = new QDockWidget(QStringLiteral("المشكلات"), this);
    diagnosticsDock->setObjectName(QStringLiteral("DiagnosticsDock"));
    // diagnosticsDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea);
    diagnosticsDock->setFeatures(QDockWidget::DockWidgetMovable
                                 | QDockWidget::DockWidgetFloatable
                                 | QDockWidget::DockWidgetClosable);
    diagnosticsPanel = new DiagnosticsPanel(diagnosticsDock);
    diagnosticsDock->setWidget(diagnosticsPanel);
    addDockWidget(Qt::BottomDockWidgetArea, diagnosticsDock);
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

    cursorPositionLabel = new QLabel(this);
    cursorPositionLabel->setStyleSheet("QLabel{ color: #f1f5f9;}");
    cursorPositionLabel->setText("UTF-8  السطر: 1  العمود: 1");
    statusBar()->addPermanentWidget(cursorPositionLabel);
}

void Taif::setupConnections() {

    connect(fileTreeView, &QTreeView::doubleClicked, this, &Taif::onFileTreeDoubleClicked);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &Taif::closeTab);
    connect(toggleSidebarAction, &QAction::triggered, this, &Taif::toggleSidebar);

    connect(menuBar, &TMenuBar::newRequested, this, &Taif::newFile);
    connect(menuBar, &TMenuBar::openFileRequested, this, [this](){this->openFile("");});
    connect(menuBar, &TMenuBar::saveRequested, this, &Taif::saveFile);
    connect(menuBar, &TMenuBar::saveAsRequested, this, &Taif::saveFileAs);
    connect(menuBar, &TMenuBar::settingsRequest, this, &Taif::openSettings);
    connect(menuBar, &TMenuBar::exitRequested, this, &Taif::exitApp);
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

    connect(menuBar, &TMenuBar::undoRequested, this, [this](){ if (auto e = currentEditor()) e->undo(); });
    connect(menuBar, &TMenuBar::redoRequested, this, [this](){ if (auto e = currentEditor()) e->redo(); });
    connect(menuBar, &TMenuBar::cutRequested, this, [this](){ if (auto e = currentEditor()) e->cut(); });
    connect(menuBar, &TMenuBar::copyRequested, this, [this](){ if (auto e = currentEditor()) e->copy(); });
    connect(menuBar, &TMenuBar::pasteRequested, this, [this](){ if (auto e = currentEditor()) e->paste(); });
    connect(menuBar, &TMenuBar::findRequested, this, &Taif::showFindBar);
    connect(menuBar, &TMenuBar::replaceRequested, this, &Taif::showReplaceBar);
    connect(menuBar, &TMenuBar::goToLineRequested, this, &Taif::goToLine);
    connect(menuBar, &TMenuBar::toggleCommentRequested, this, [this](){ if (auto e = currentEditor()) e->toggleComment(); });
    connect(menuBar, &TMenuBar::duplicateLineRequested, this, [this](){ if (auto e = currentEditor()) e->duplicateLine(); });
    connect(menuBar, &TMenuBar::moveLineUpRequested, this, [this](){ if (auto e = currentEditor()) e->moveLineUp(); });
    connect(menuBar, &TMenuBar::moveLineDownRequested, this, [this](){ if (auto e = currentEditor()) e->moveLineDown(); });

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

    connect(searchBar, &SearchPanel::findText, this, &Taif::findText);
    connect(searchBar, &SearchPanel::findNext, this, &Taif::findNextText);
    connect(searchBar, &SearchPanel::findPrevious, this, &Taif::findPrevText);
    connect(searchBar, &SearchPanel::closed, this, &Taif::hideFindBar);
    connect(searchBar, &SearchPanel::replaceOne, this, &Taif::replaceOne);
    connect(searchBar, &SearchPanel::replaceAll, this, &Taif::replaceAll);
}

void Taif::setupStyle() {
    QString styleSheet = R"(
        QMainWindow {
            background-color: #0f172a;
            font-size: 13px;
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
        QTabBar { /* شريط التبويبات */
            background-color: #0f172a;
            qproperty-drawBase: 0;
        }
        QTabBar::tab {
            background: #0f172a;
            color: #94a3b8;
            padding: 3px 9px;
            min-width: 100px;
        }
        QTabBar::tab:selected {
            background: #0f172a;
            color: #3b82f6;
            border-bottom: 2px solid #3b82f6;
        }
        QTabBar::tab:hover:!selected {
            background: #334466;
            color: #f1f5f9;
        }
        QTabBar::close-button {
            image: url(:/icons/resources/close.svg);
            background: transparent;
            border-radius: 2px;
            padding: 1px;
            margin: 0px;
        }
        QTabBar::close-button:hover {
            background: #ef4444;
        }

        /* --- شريط الحالة --- */
        QStatusBar {
            background-color: #0f172a;
            color: #64748b;
            border-top: 1px solid #1e293b;
            font-size: 11px;
            padding-left: 10px;
        }

        /* --- نوافذ قابلة للدكن --- */
        QDockWidget {
            background-color: #0f172a;
            color: #e2e8f0;
            font-family: "Tajawal", "Noto Kufi Arabic";
            border-top: 1px solid #334155;
            padding: 1px;
        }
        QDockWidget::title {
            background-color: #1e293b;
            color: #e2e8f0;
            text-align: left;
            padding: 9px;
            border-bottom: 1px solid #334155;
        }
        QDockWidget::close-button,
        QDockWidget::float-button {
            background: transparent;
            border: none;
            border-radius: 3px;
            padding: 1px;
            margin: 8px;
            icon-size: 0px;
            width: 0px;
            height: 0px;
            subcontrol-position: top right;
            position: absolute;
        }
        QDockWidget::close-button {
            image: url(:/icons/resources/close.svg);
            top: 9px; right: 7px; bottom: 9px;
        }
        QDockWidget::float-button {
            image: url(:/icons/resources/picture-in-picture.svg);
            top: 9px; right: 35px; bottom: 9px;
        }
        QDockWidget::close-button:hover {
            background-color: #ef4444;
        }
        QDockWidget::float-button:hover {
            background-color: #334466;
        }
    )";
    setStyleSheet(styleSheet);
}

void Taif::setupTimers() {

    // file watcher timer
    saveSuppressTimer = new QTimer(this);
    saveSuppressTimer->setSingleShot(true);
    connect(saveSuppressTimer, &QTimer::timeout, this, [this]{ savingFromApp = false; });


}

void Taif::closeEvent(QCloseEvent *event) {
    int saveResult = needSave();

    if (saveResult == 1) {
        saveFile();
    } else if (saveResult == 0) {
        event->ignore();
        return;
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

    // أقصى رقم هو عدد أسطر الملف الحالي
    const int maxLine = editor->blockCount();

    QDialog dlg(this);
    dlg.setWindowTitle("الذهاب إلى سطر");
    dlg.setFixedWidth(300);
    dlg.setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    QLabel *label = new QLabel(QString("أدخل رقم السطر (1 - %1):").arg(maxLine), &dlg);

    QSpinBox *spin = new QSpinBox(&dlg);
    spin->setRange(1, maxLine);
    spin->setValue(editor->textCursor().blockNumber() + 1);
    spin->setKeyboardTracking(true);
    spin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    spin->setFocus();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(&dlg);
    QPushButton *okButton = buttonBox->addButton("إذهب", QDialogButtonBox::AcceptRole);
    QPushButton *cancelButton = buttonBox->addButton("إلغاء", QDialogButtonBox::RejectRole);
    okButton->setDefault(true);

    connect(buttonBox, &QDialogButtonBox::accepted,
            &dlg, &QDialog::accept);

    connect(buttonBox, &QDialogButtonBox::rejected,
            &dlg, &QDialog::reject);

    layout->addWidget(label);
    layout->addWidget(spin);
    layout->addWidget(buttonBox);

    // نمط مطابق لنمط المحرر (داكن + خط عربي)
    dlg.setStyleSheet(R"(
        QDialog {
            background-color: #0f172a;
            font-family: "Tajawal", "Noto Kufi Arabic";
        }
        QLabel {
            color: #f1f5f9;
            font-size: 13px;
        }
        QSpinBox {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            border-radius: 4px;
            padding: 6px 10px;
            font-size: 13px;
            selection-background-color: #3b82f6;
        }
        QSpinBox:focus {
            border: 1px solid #3b82f6;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            width: 20px;
            border-radius: 3px;
            background-color: #1e293b;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #1c7ed6;
        }
        QSpinBox::up-arrow {
            image: url(:/icons/resources/chevron-up.svg);
            width: 16px;
        }
        QSpinBox::down-arrow {
            image: url(:/icons/resources/chevron-down.svg);
            width: 16px;
        }
        QDialogButtonBox {
            spacing: 10px;
        }
        QDialogButtonBox QPushButton {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            border-radius: 4px;
            padding: 6px 20px;
            font-family: "Tajawal", "Noto Kufi Arabic";
            font-size: 13px;
        }
        QDialogButtonBox QPushButton:hover {
            background-color: #334155;
        }
        QDialogButtonBox QPushButton:pressed {
            background-color: #0f172a;
        }
        QDialogButtonBox QPushButton:focus {
            border: 1px solid #3b82f6;
        }
    )");

    // زر الموافق بلون مميز (الأزرق)
    okButton->setStyleSheet(
        "QPushButton{background-color:#3b82f6; border:none; color:#ffffff;}"
        "QPushButton:hover{background-color:#2563eb;}"
        "QPushButton:pressed{background-color:#1d4ed8;}");

    if (dlg.exec() == QDialog::Accepted) {
        const int lineNumber = spin->value();
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

void Taif::showFindBar()
{
    if (TEditor *editor = currentEditor()) {
        searchBar->showIn(editor);
        searchBar->showReplaceRow(false);
        searchBar->setFocusToInput();
    }
}

void Taif::showReplaceBar()
{
    if (TEditor *editor = currentEditor()) {
        searchBar->showIn(editor);
        searchBar->showReplaceRow(true);
        searchBar->setFocusToInput();
    }
}

void Taif::hideFindBar()
{
    searchBar->hide();
    clearSearchHighlights();
    if (TEditor *editor = currentEditor())
        editor->setFocus(Qt::OtherFocusReason);
}

void Taif::clearSearchHighlights() {
    if (TEditor* editor = currentEditor()) {
        editor->setExtraSelections({});
    }
}

static bool isWordCharacter(const QChar ch)
{
    return ch.isLetterOrNumber() || ch == u'_';
}

static bool isWholeWordMatch(const QString &text, const int start, const int length)
{
    const int end = start + length;
    const bool leftWord = start > 0 && isWordCharacter(text.at(start - 1));
    const bool rightWord = end < text.size() && isWordCharacter(text.at(end));
    return !leftWord && !rightWord;
}

static QList<QPair<int, int>> collectMatches(
    TEditor *editor, const QString &pattern, const bool caseSensitive,
    const bool wholeWord, const bool useRegex)
{
    QList<QPair<int, int>> matches;
    if (!editor || pattern.isEmpty())
        return matches;

    const QString text = editor->document()->toPlainText();
    if (useRegex) {
        QRegularExpression expression(
            pattern, caseSensitive ? QRegularExpression::NoPatternOption
                          : QRegularExpression::CaseInsensitiveOption);
        if (!expression.isValid())
            return matches;

        auto iterator = expression.globalMatch(text);
        while (iterator.hasNext()) {
            const QRegularExpressionMatch match = iterator.next();
            const int start = match.capturedStart();
            const int length = match.capturedLength();
            if (length == 0)
                continue;
            if (!wholeWord || isWholeWordMatch(text, start, length))
                matches.append({start, length});
        }
        return matches;
    }

    const Qt::CaseSensitivity sensitivity = caseSensitive
                                                ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int position = 0;
    while ((position = text.indexOf(pattern, position, sensitivity)) >= 0) {
        if (!wholeWord || isWholeWordMatch(text, position, pattern.size()))
            matches.append({position, pattern.size()});
        position += qMax(1, pattern.size());
    }
    return matches;
}

// Highlight every match in the document and optionally select the current one.
static void applyMatchHighlights(TEditor* editor, const QList<QPair<int, int>>& matches, int currentIdx)
{
    QList<QTextEdit::ExtraSelection> selections;
    const QColor matchColor(245, 158, 11, 70);     // dim amber for all matches
    const QColor currentColor(245, 158, 11, 160);   // bright amber for current

    for (int i = 0; i < matches.size(); ++i) {
        QTextEdit::ExtraSelection sel;
        QTextCursor c = editor->textCursor();
        c.setPosition(matches[i].first);
        c.setPosition(matches[i].first + matches[i].second, QTextCursor::KeepAnchor);
        sel.cursor = c;
        sel.format.setBackground(i == currentIdx ? currentColor : matchColor);
        selections.append(sel);
    }
    editor->setExtraSelections(selections);
}


void Taif::performSearch(bool forward, bool next) {
    TEditor* editor = currentEditor();
    QString text = searchBar->searchText();
    if (!editor || text.isEmpty()) return;

    bool cs = searchBar->isCaseSensitive();
    bool ww = searchBar->isWholeWord();
    bool rx = searchBar->isRegex();

    // Validate regex before searching
    if (rx) {
        QRegularExpression re(text, cs ? QRegularExpression::NoPatternOption
                                      : QRegularExpression::CaseInsensitiveOption);
        if (!re.isValid()) {
            searchBar->setMatchInfo(0, 0);
            searchBar->setNoMatchesFound(true);
            return;
        }
    }

    auto matches = collectMatches(editor, text, cs, ww, rx);

    if (matches.isEmpty()) {
        clearSearchHighlights();
        searchBar->setMatchInfo(0, 0);
        searchBar->setNoMatchesFound(true);
        return;
    }

    const QTextCursor editorCursor = editor->textCursor();
    const int cursorStart = editorCursor.selectionStart();
    const int cursorEnd = editorCursor.selectionEnd();

    int currentIdx = 0;
    if (!next) {
        // A new search or changed option must not advance an already selected match.
        // QTextCursor::position() is the selection end, so using it directly would
        // incorrectly move to the following match when Aa/كلمة is clicked.
        const int selectionStart = editorCursor.selectionStart();
        const int selectionEnd = editorCursor.selectionEnd();
        bool selectedMatchFound = false;
        for (int i = 0; i < matches.size(); ++i) {
            if (matches.at(i).first == selectionStart &&
                matches.at(i).first + matches.at(i).second == selectionEnd) {
                currentIdx = i;
                selectedMatchFound = true;
                break;
            }
        }

        if (!selectedMatchFound) {
            // Initial search: select the first match at or after the caret.
            const int anchor = selectionStart;
            currentIdx = 0;
            for (int i = 0; i < matches.size(); ++i) {
                if (matches.at(i).first >= anchor) {
                    currentIdx = i;
                    break;
                }
                currentIdx = i;
            }
        }
    } else if (forward) {
        // Next: move strictly after the currently selected match.
        currentIdx = 0;
        bool foundNext = false;
        for (int i = 0; i < matches.size(); ++i) {
            if (matches.at(i).first >= cursorEnd) {
                currentIdx = i;
                foundNext = true;
                break;
            }
        }
        if (!foundNext)
            currentIdx = 0; // wrap to the first match
    } else {
        // Previous: move strictly before the currently selected match.
        currentIdx = matches.size() - 1;
        for (int i = matches.size() - 1; i >= 0; --i) {
            if (matches.at(i).first < cursorStart) {
                currentIdx = i;
                break;
            }
        }
    }

    applyMatchHighlights(editor, matches, currentIdx);
    QTextCursor matchCursor = editor->textCursor();
    matchCursor.setPosition(matches.at(currentIdx).first);
    matchCursor.setPosition(matches.at(currentIdx).first + matches.at(currentIdx).second,
                            QTextCursor::KeepAnchor);
    editor->setTextCursor(matchCursor);
    editor->ensureCursorVisible();
    searchBar->setMatchInfo(currentIdx + 1, matches.size());
    searchBar->setNoMatchesFound(false);
}

void Taif::findText()       { performSearch(true,  false); }
void Taif::findNextText()   { performSearch(true,  true); }
void Taif::findPrevText()   { performSearch(false, true); }

void Taif::replaceOne() {
    TEditor* editor = currentEditor();
    QString search = searchBar->searchText();
    QString replace = searchBar->replaceText();
    if (!editor || search.isEmpty()) return;

    bool cs = searchBar->isCaseSensitive();
    bool ww = searchBar->isWholeWord();
    bool rx = searchBar->isRegex();

    if (rx) {
        QRegularExpression re(search, cs ? QRegularExpression::NoPatternOption
                                        : QRegularExpression::CaseInsensitiveOption);
        if (!re.isValid()) return;
    }

    auto matches = collectMatches(editor, search, cs, ww, rx);
    if (matches.isEmpty()) return;

    // Find the match that contains the cursor or starts at the selection.
    int cursorPos = editor->textCursor().selectionStart();
    int targetIdx = -1;
    for (int i = 0; i < matches.size(); ++i) {
        const int matchStart = matches[i].first;
        const int matchEnd = matchStart + matches[i].second;
        if (matchStart <= cursorPos && cursorPos < matchEnd) {
            targetIdx = i;
            break;
        }
    }
    if (targetIdx == -1) targetIdx = 0;

    // Perform the replacement
    QString replacement;
    if (rx) {
        QRegularExpression re(search, cs ? QRegularExpression::NoPatternOption
                                        : QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = re.match(editor->document()->toPlainText().mid(matches[targetIdx].first, matches[targetIdx].second));
        replacement = match.hasMatch() ? match.captured(0).replace(re, replace) : replace;
    } else {
        replacement = replace;
    }

    QTextCursor c = editor->textCursor();
    c.setPosition(matches[targetIdx].first);
    c.setPosition(matches[targetIdx].first + matches[targetIdx].second, QTextCursor::KeepAnchor);
    c.insertText(replacement);

    // Re-search to update highlights and move to next match
    performSearch(true, true);
}

void Taif::replaceAll() {
    TEditor* editor = currentEditor();
    QString search = searchBar->searchText();
    QString replace = searchBar->replaceText();
    if (!editor || search.isEmpty()) return;

    bool cs = searchBar->isCaseSensitive();
    bool ww = searchBar->isWholeWord();
    bool rx = searchBar->isRegex();

    if (rx) {
        QRegularExpression re(search, cs ? QRegularExpression::NoPatternOption
                                        : QRegularExpression::CaseInsensitiveOption);
        if (!re.isValid()) return;
    }

    auto matches = collectMatches(editor, search, cs, ww, rx);
    if (matches.isEmpty()) return;

    // Group the complete Replace All operation into one undo action.
    QTextCursor editCursor(editor->document());
    editCursor.beginEditBlock();

    if (rx) {
        QRegularExpression re(search, cs ? QRegularExpression::NoPatternOption
                                        : QRegularExpression::CaseInsensitiveOption);
        const QString text = editor->toPlainText();

        // Replace only the matches collected above so whole-word filtering is respected.
        // Process from end to start to keep document positions valid.
        for (int i = matches.size() - 1; i >= 0; --i) {
            const QString matchedText = text.mid(matches[i].first, matches[i].second);
            const QRegularExpressionMatch match = re.match(matchedText);
            const QString replacement = match.hasMatch()
                                            ? match.captured(0).replace(re, replace)
                                            : replace;

            QTextCursor c = editor->textCursor();
            c.setPosition(matches[i].first);
            c.setPosition(matches[i].first + matches[i].second, QTextCursor::KeepAnchor);
            c.insertText(replacement);
        }
    } else {
        // Replace from end to start to keep positions valid
        for (int i = matches.size() - 1; i >= 0; --i) {
            QTextCursor c = editor->textCursor();
            c.setPosition(matches[i].first);
            c.setPosition(matches[i].first + matches[i].second, QTextCursor::KeepAnchor);
            c.insertText(replace);
        }
    }

    editCursor.endEditBlock();

    // Update highlights after replacement
    auto newMatches = collectMatches(editor, search, cs, ww, rx);
    if (newMatches.isEmpty()) {
        clearSearchHighlights();
        searchBar->setMatchInfo(0, 0);
        searchBar->setNoMatchesFound(true);
    } else {
        applyMatchHighlights(editor, newMatches, 0);
        QTextCursor c = editor->textCursor();
        c.setPosition(newMatches[0].first);
        c.setPosition(newMatches[0].first + newMatches[0].second, QTextCursor::KeepAnchor);
        editor->setTextCursor(c);
        editor->ensureCursorVisible();
        searchBar->setMatchInfo(1, newMatches.size());
        searchBar->setNoMatchesFound(false);
    }
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

void Taif::showAndRaiseDock(QDockWidget* const dock) {
    if (dock == nullptr) {
        return;
    }

    if (dock == terminalDock || dock == alifOutputDock) {
        DockableConsoleToolFactory::ensureTabifiedWith(this, diagnosticsDock, dock);
    }
    DockableConsoleToolFactory::showAndActivate(dock);
    QTimer::singleShot(0, this, &Taif::syncBottomToolActionState);
}

void Taif::toggleConsole() {
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

int Taif::needSave() {
    if (TEditor* editor = currentEditor()) {
        if (editor->document()->isModified()) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("طيف");
            msgBox.setText("تم التعديل على الملف.\n"    \
                           "هل تريد حفظ التغييرات؟");
            QPushButton *saveButton = msgBox.addButton("حفظ", QMessageBox::AcceptRole);
            QPushButton *discardButton = msgBox.addButton("تجاهل", QMessageBox::DestructiveRole);
            QPushButton *cancelButton = msgBox.addButton("إلغاء", QMessageBox::RejectRole);
            msgBox.setDefaultButton(cancelButton);

            QFont msgFont = this->font();
            msgFont.setPixelSize(12);
            saveButton->setFont(msgFont);
            discardButton->setFont(msgFont);
            cancelButton->setFont(msgFont);

            msgBox.exec();

            QAbstractButton *clickedButton = msgBox.clickedButton();
            if (clickedButton == saveButton) {
                return 1;
            } else if (clickedButton == discardButton) {
                return 2;
            } else if (clickedButton == cancelButton) {
                return 0;
            }
        }
    }

    return 2;
}

void Taif::newFile() {

    TEditor* editor = currentEditor();
    if (editor) {
        int isNeedSave = needSave();
        if (!isNeedSave) return;
        if (isNeedSave == 1) this->saveFile();
    }

    TEditor *newEditor = new TEditor(setting, this);
    tabWidget->addTab(newEditor, "غير معنون");
    tabWidget->setCurrentWidget(newEditor);

    connect(newEditor, &TEditor::openRequest, this, [this](QString filePath){this->openFile(filePath);});
    connectEditorDiagnostics(newEditor);
    connect(newEditor->document(), &QTextDocument::modificationChanged, this, &Taif::onModificationChanged);
    updateWindowTitle();
}

void Taif::addWatch(const QString &filePath)
{
    if (filePath.isEmpty() || !QFile::exists(filePath)) return;

    if (!fileWatcher) {
        fileWatcher = new QFileSystemWatcher(this);
        connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, &Taif::onFileChanged);
    }

    const QString watchPath = QFileInfo(filePath).absoluteFilePath();
    if (!fileWatcher->files().contains(watchPath)) {
        fileWatcher->addPath(watchPath);
    }
}

void Taif::removeWatch(const QString &filePath)
{
    if (filePath.isEmpty() || !fileWatcher) return;
    const QString watchPath = QFileInfo(filePath).absoluteFilePath();
    if (fileWatcher->files().contains(watchPath)) {
        fileWatcher->removePath(watchPath);
    }
}

QString Taif::pathForEditor(TEditor *editor) const
{
    if (!editor) return QString();
    return QFileInfo(editor->property("filePath").toString()).absoluteFilePath();
}

void Taif::reloadEditor(TEditor *editor)
{
    const QString filePath = editor->property("filePath").toString();
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // قد يكون الملف قيد الكتابة أو تم حذفه مؤقتًا؛ سنعيد مراقبته
        addWatch(filePath);
        return;
    }
    QTextStream in(&file);
    const QString content = in.readAll();
    file.close();

    const int index = tabWidget->indexOf(editor);
    if (index == -1) return;

    // حفظ موقع المؤشر النسبي قدر الإمكان
    const int cursorPos = editor->textCursor().position();

    editor->document()->setModified(false);
    editor->setPlainText(content);

    // استعادة أقرب موضع للمؤشر داخل النطاق الجديد
    const int newPos = qBound(0, cursorPos, editor->document()->characterCount() - 1);
    QTextCursor cursor = editor->textCursor();
    cursor.setPosition(newPos);
    editor->setTextCursor(cursor);

    if (index == tabWidget->currentIndex()) {
        updateCursorPosition();
    }
    updateWindowTitle();

    // QFileSystemWatcher يُزيل المسار تلقائيًا بعد الحدث، لذا نعيده
    addWatch(filePath);
}

void Taif::onFileChanged(const QString &path)
{
    // تجاهل التغيّرات الناتجة عن حفظنا نحن داخل البرنامج
    if (savingFromApp) {
        addWatch(path);
        return;
    }

    const QString watchPath = QFileInfo(path).absoluteFilePath();

    // QFileSystemWatcher يُزيل المسار بعد الحدث، نعيد إضافته للمتابعة
    addWatch(watchPath);

    for (int i = 0; i < tabWidget->count(); ++i) {
        TEditor* editor = qobject_cast<TEditor*>(tabWidget->widget(i));
        if (!editor) continue;
        if (pathForEditor(editor) != watchPath) continue;

        if (editor->document()->isModified()) {
            if (i == tabWidget->currentIndex()) {
                QMessageBox msgBox(this);
                msgBox.setWindowTitle("طيف");
                msgBox.setText(QString("<div align='right'>%1"
                                       "<br><br>"
                                       "تم تعديل هذا الملف في برنامح آخر."
                                       "<br>"
                                       "هل تريد إعادة تحميل النسخة المعدلة من القرص؟</div>").arg(watchPath));
                QPushButton *reloadBtn = msgBox.addButton("إعادة تحميل", QMessageBox::AcceptRole);
                /*QPushButton *keepBtn =*/ msgBox.addButton("أبقي نسختي", QMessageBox::RejectRole);
                msgBox.exec();
                if (msgBox.clickedButton() == reloadBtn) {
                    reloadEditor(editor);
                }
            }
        } else {
            reloadEditor(editor);
        }
    }
}

void Taif::openFile(QString filePath)
{
    if (TEditor* const current = currentEditor()) {
        const int saveDecision = needSave();
        if (saveDecision == 0) {
            return;
        }
        if (saveDecision == 1) {
            saveFile();
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
    newEditor->setPlainText(content);
    newEditor->setProperty("filePath", filePath);

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
    connect(newEditor->document(), &QTextDocument::modificationChanged,
            this, &Taif::onModificationChanged);
    connectEditorDiagnostics(newEditor);
    // connectEditorActionState(newEditor); //* review

    const QFileInfo fileInfo(filePath);
    const int tabIndex = tabWidget->addTab(newEditor, fileInfo.fileName());
    tabWidget->setTabToolTip(tabIndex, filePath);
    if (activateTab) {
        tabWidget->setCurrentIndex(tabIndex);
    }

    if (updateRecentFiles) {
        QSettings settings(QStringLiteral("Alif"), QStringLiteral("Taif"));
        QStringList recentFiles = settings.value(QStringLiteral("RecentFiles")).toStringList();
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);
        while (recentFiles.size() > 10) {
            recentFiles.removeLast();
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
    // updateEditActionState(); //* review
    refreshBreadcrumbs();
    return result;
}

void Taif::loadFolder(const QString& requestedFolderPath) {
    const QString normalizedFolderPath = SessionStore::normalizePath(requestedFolderPath);
    if (!normalizedFolderPath.isEmpty() && QDir(normalizedFolderPath).exists()) {
        folderPath = normalizedFolderPath;
        fileTreeView->setVisible(true);

        fileTreeView->setRootIndex(fileSystemModel->index(folderPath));
    } else {
        folderPath.clear();
        fileTreeView->setVisible(false);
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
    bool shouldBeVisible = !fileTreeView->isVisible();
    fileTreeView->setVisible(shouldBeVisible);
    toggleSidebarAction->setChecked(shouldBeVisible);

    if (shouldBeVisible && fileTreeView->rootIndex() == QModelIndex()) {
        QString homePath = QDir::homePath();
        fileTreeView->setRootIndex(fileSystemModel->index(homePath));
    }
}

void Taif::onFileTreeDoubleClicked(const QModelIndex &index)
{
    const QString filePath = fileSystemModel->filePath(index);
    if (!fileSystemModel->isDir(index)) {
        openFile(filePath);
    }
}

void Taif::saveFile() {
    TEditor *editor = currentEditor();
    if (!editor) return;

    QString filePath = editor->property("filePath").toString();
    // --------------------------------------------------------

    QString content = editor->toPlainText();

    if (filePath.isEmpty()) {
        saveFileAs();
        return;
    } else {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            out << content;
            file.close();
            editor->document()->setModified(false);

            // نتجاهل حدث التغيير الناتج عن حفظنا نحن (يصل بشكل غير متزامن)
            if (fileWatcher) {
                savingFromApp = true;
                saveSuppressTimer->start(500);
            }

            int index = tabWidget->indexOf(editor);
            if (index != -1) {
                QFileInfo fileInfo(filePath);
                tabWidget->setTabText(index, fileInfo.fileName());
            }
            editor->removeBackupFile();
            updateWindowTitle();
            refreshBreadcrumbs();
            return ;
        } else {
            QMessageBox::warning(this, "خطأ", "لا يمكن حفظ الملف");
            return;
        }
    }
}

void Taif::saveFileAs() {
    TEditor *editor = currentEditor();
    if (!editor) return ;

    QString content = editor->toPlainText();
    QString currentPath = editor->property("filePath").toString();
    QString currentName = currentPath.isEmpty() ? "ملف جديد.alif" : QFileInfo(currentPath).fileName();
    QString fileName = QFileDialog::getSaveFileName(this, "حفظ الملف", currentName, "ملف ألف (*.alif);;مكتبة ألف(*.aliflib);;All Files (*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            out << content;
            file.close();

            // نتجاهل حدث التغيير الناتج عن حفظنا نحن (يصل بشكل غير متزامن)
            if (fileWatcher) {
                savingFromApp = true;
                saveSuppressTimer->start(500);
            }

            editor->setProperty("filePath", fileName);
            removeWatch(currentPath);
            addWatch(fileName);
            // ---------------------------------------------------

            editor->document()->setModified(false);

            int index = tabWidget->indexOf(editor);
            if (index != -1) {
                QFileInfo fileInfo(fileName);
                tabWidget->setTabText(index, fileInfo.fileName());
            }

            updateWindowTitle();
            refreshBreadcrumbs();
            return ;
        } else {
            QMessageBox::warning(this, "خطأ", "لا يمكن حفظ الملف");
            return ;
        }
    }
    return ;
}

void Taif::openSettings() {
    if (setting and setting->isVisible()) return;

    connect(setting, &TSettings::fontSizeChanged, this, [this](int size){
        for (int i = 0; i < tabWidget->count(); ++i) {
            qobject_cast<TEditor*>(tabWidget->widget(i))->updateFontSize(size);
        }
    });
    connect(setting, &TSettings::fontTypeChanged, this, [this](QString font){
        for (int i = 0; i < tabWidget->count(); ++i) {
            qobject_cast<TEditor*>(tabWidget->widget(i))->updateFontType(font);
        }
    });
    connect(setting, &TSettings::highlighterThemeChanged, this, [this](int themeIdx){
        QVector<std::shared_ptr<SyntaxTheme>> availableThemes = setting->getAvailableThemes();
        std::shared_ptr<SyntaxTheme> theme = availableThemes.at(themeIdx);
        for (int i = 0; i < tabWidget->count(); ++i) {
            qobject_cast<TEditor*>(tabWidget->widget(i))->updateHighlighterTheme(theme);
        }
    });

    setting->show();
}


void Taif::exitApp() {
    int isNeedSave = needSave();
    if (!isNeedSave) {
        return;
    }
    else if (isNeedSave == 1) {
        this->saveFile();
        return;
    }

    WelcomeWindow *welcome = new WelcomeWindow();
    welcome->show();
    this->close();
}

void Taif::onCurrentTabChanged()
{
    updateWindowTitle();
    updateCursorPosition();

    TEditor* editor = currentEditor();
    if (editor) {
        connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &Taif::updateCursorPosition);
    }
    refreshDiagnosticsPanel();
    // updateEditActionState(); //* review
    bindBreadcrumbsToEditor(editor);
    refreshBreadcrumbs();
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
    const QModelIndex index = fileSystemModel->index(pathInfo.absoluteFilePath());
    if (!index.isValid()) {
        return;
    }
    fileTreeView->setVisible(true);
    fileTreeView->expand(index);
    fileTreeView->scrollTo(index);
    fileTreeView->setCurrentIndex(index);
}

void Taif::updateCursorPosition()
{
    TEditor* editor = currentEditor();
    if (editor) {
        const QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int column = cursor.columnNumber() + 1;

        cursorPositionLabel->setText(QString("UTF-8    السطر: %1   العمود: %2 ").arg(line).arg(column));
    } else {
        cursorPositionLabel->setText("");
    }
}


/* ----------------------------------- Run Menu Button ----------------------------------- */

void Taif::runAlif() {
    TEditor *editor = currentEditor();
    if (!editor) return;

    QString filePath = editor->property("filePath").toString();
    if (filePath.isEmpty() || editor->document()->isModified()) {
        QMessageBox::warning(this, "تنبيه", "يجب حفظ الملف قبل التشغيل.");
        saveFile();
        filePath = editor->property("filePath").toString();
        if (filePath.isEmpty() || editor->document()->isModified()) return;
    }

    TConsole* console = alifOutputConsole;
    if (console == nullptr || alifOutputDock == nullptr) {
        return;
    }

    showAndRaiseDock(alifOutputDock);

    QString program;
    QString appDir = QCoreApplication::applicationDirPath();

#if defined(Q_OS_WIN)
    QString localAlif = QDir(appDir).filePath("alif/alif.exe");
    qDebug() <<  " -------------------------------------------------------------------------------------------- "  << localAlif <<  " -------------------------------------------------------------------------------------------- ";

    if (QFile::exists(localAlif)) {
        program = localAlif;
    } else {
        program = "alif/alif.exe";
    }
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    program = QDir(appDir).filePath("alif/alif");
#endif

    if (!QFile::exists(program)) {
        console->clear();
        console->appendPlainTextThreadSafe("❌ خطأ: لم يتم العثور على مترجم ألف!");
        console->appendPlainTextThreadSafe("المسار المتوقع: " + program);

#if defined(Q_OS_LINUX)
        console->appendPlainTextThreadSafe("تأكد من أن ملف 'alif' موجود ولديه صلاحية التنفيذ (chmod +x).");
#endif
        return;
    }

    QStringList args = { filePath };
    QString workingDir = QFileInfo(filePath).absolutePath();

    if (worker) {
        worker->disconnect();
        worker->deleteLater();
        worker = nullptr;
    }

    console->clear();

    // Fetch and print the version (alif -ن)
    QProcess versionCheck{};
    versionCheck.setWorkingDirectory(QFileInfo(program).absolutePath());
    versionCheck.start(program, {"-ن"});

    // We wait briefly (max 2s) for the version check as it's a fast command
    if (versionCheck.waitForFinished(2000)) {
        QString versionOutput = versionCheck.readAll();
        if (!versionOutput.isEmpty()) {
            console->appendPlainTextThreadSafe(versionOutput);
        }
    }

    console->appendPlainTextThreadSafe("🚀 بدء تشغيل ملف ألف: " + QFileInfo(filePath).fileName() + "\n\n");

    worker = new ProcessWorker(program, args, workingDir);
    QThread *thread = new QThread();

    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &ProcessWorker::start);
    connect(worker, &ProcessWorker::outputReady,
            console, &TConsole::appendPlainTextThreadSafe);
    connect(worker, &ProcessWorker::errorReady,
            console, &TConsole::appendPlainTextThreadSafe);

    connect(worker, &ProcessWorker::finished, this, [=](int code){
        console->appendPlainTextThreadSafe(
            "\n──────────────────────────────\n✅ انتهى التنفيذ (رمز الخروج = "
            + QString::number(code) + ")\n"
            );
        thread->quit();
    });

    connect(worker, &QObject::destroyed, this, [this]() {
        worker = nullptr;
    });
    // Clean up thread and worker memory when finished
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(worker, &QObject::destroyed, thread, &QThread::quit); // Quit thread when worker is gone
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);

    connect(console, &TConsole::commandEntered,
            worker, &ProcessWorker::sendInput);

    thread->start();
}

//----------------

TEditor* Taif::currentEditor() {
    return qobject_cast<TEditor*>(tabWidget->currentWidget());
}

void Taif::closeTab(int index)
{

    if (tabWidget->count() <= 1) {
        return;
    }

    QWidget *tab = tabWidget->widget(index);

    if (!tab) return;

    TEditor* editor = qobject_cast<TEditor*>(tabWidget->widget(index));
    if (!editor) return;

    if (editor && editor->document()->isModified()) {
        int saveResult = needSave();

        if (!saveResult) {
            return;
        }
        else if (saveResult == 1) {
            this->saveFile();
            return;
        }

    }
    removeWatch(editor->property("filePath").toString());
    tabWidget->removeTab(index);

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

void Taif::onModificationChanged(bool modified) {
    updateWindowTitle(); // استدعِ الدالة لتحديث علامة [*]
    // قد تحتاج أيضًا لتحديث اسم التبويب نفسه لإضافة [*]
    TEditor* editor = currentEditor(); // الحصول على المحرر المرتبط بالإشارة
    if (editor) {
        int index = tabWidget->indexOf(editor);
        if (index != -1) {
            QString currentText = tabWidget->tabText(index);
            if (modified && !currentText.endsWith("[*]")) {
                tabWidget->setTabText(index, currentText + "[*]");
            } else if (!modified && currentText.endsWith("[*]")) {
                tabWidget->setTabText(index, currentText.left(currentText.length() - 3));
            }
        }
    }
}

