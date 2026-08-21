#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtGui/QTextDocument>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QUuid>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QApplication>

#include "DockableConsoleTool.h"
#include "TConsole.h"
#include "TMenu.h"
#include "TSearchPanel.h"
#include "SearchReplaceEngine.h"
#include "SessionEditorDialog.h"
#include "SessionStore.h"
#include "TBreadcrumbBar.h"
#include "ApplicationBootstrap.h"
#include "EditorPreferences.h"
#include "TSettings.h"
#include "RecoveryStore.h"
#include "RecoveryCoordinator.h"
#include "TRecoveryDialog.h"
#include "TEditor.h"
#include "AlifRunController.h"

#include <QTemporaryDir>
#include <QTreeWidget>
#include <QPushButton>
#include <utility>

class DockableToolsTest final : public QObject {
    Q_OBJECT

private slots:
    void bottomToolsArePersistentAndTabified();
    void activationSelectsRequestedBottomToolTab();
    void viewMenuExposesOrderedDockActions();
    void editMenuExposesOrderedCommandActions();
    void searchPanelProvidesReplaceSurface();
    void searchReplaceEnginePreservesMatchAndUndoSemantics();
    void sessionStorePersistsNormalizedSessions();
    void sessionEditorPreservesOrderedFilesInRtl();
    void breadcrumbBarUsesRtlAndRepresentsUntitledFile();
    void breadcrumbBarRendersOrderedFileAndSemanticSegments();
    void breadcrumbBarClearsSemanticSegmentsAndEmitsNavigationSignals();
    void applicationBootstrapProvidesStableFontRolesAndLaunchValidation();
    void editorPreferencesNormalizeInvalidValues();
    void settingsWindowUsesRtlDraftApplyCancelWorkflow();
    void recoveryStoreAtomicallyPersistsReadsAndRemovesSnapshots();
    void recoveryCoordinatorReportsRemovalFailureAndAsynchronousFlushOutcome();
    void editorRecoveryClearsDirtyOnlyAfterLatestSnapshotAcknowledgement();
    void editorRecoveryFailureKeepsDirtyAndSchedulesBoundedRetry();
    void recoveryDialogUsesRtlAndSelectsEntriesByDefault();
    void alifRunControllerValidatesRunsAndCancels();
};

void DockableToolsTest::alifRunControllerValidatesRunsAndCancels()
{
    AlifRunController controller;

    AlifRunController::Request invalidRequest;
    invalidRequest.program = QStringLiteral("Z:/missing-alif-runtime.exe");
    QString validationError;
    QVERIFY(!controller.start(invalidRequest, &validationError));
    QVERIFY(!validationError.isEmpty());
    QCOMPARE(controller.state(), AlifRunController::State::Idle);

    AlifRunController::Request quickRequest;
#if defined(Q_OS_WIN)
    quickRequest.program = qEnvironmentVariable("COMSPEC", QStringLiteral("C:/Windows/System32/cmd.exe"));
    quickRequest.arguments = {QStringLiteral("/C"), QStringLiteral("echo managed-run-output")};
#else
    quickRequest.program = QStringLiteral("/bin/sh");
    quickRequest.arguments = {QStringLiteral("-c"), QStringLiteral("printf managed-run-output")};
#endif
    quickRequest.workingDirectory = QDir::tempPath();

    QSignalSpy outputSpy(&controller, &AlifRunController::standardOutput);
    QSignalSpy finishedSpy(&controller, &AlifRunController::finished);
    QString startError;
    QVERIFY(controller.start(quickRequest, &startError));
    QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() == 1, 3000);
    QVERIFY(!outputSpy.isEmpty());
    QVERIFY(outputSpy.constFirst().constFirst().toString().contains(
        QStringLiteral("managed-run-output")));
    QVERIFY(!controller.isActive());

    AlifRunController::Request longRequest;
#if defined(Q_OS_WIN)
    longRequest.program = quickRequest.program;
    longRequest.arguments = {QStringLiteral("/C"),
                             QStringLiteral("ping -n 10 127.0.0.1 > nul")};
#else
    longRequest.program = QStringLiteral("/bin/sh");
    longRequest.arguments = {QStringLiteral("-c"), QStringLiteral("sleep 10")};
#endif
    longRequest.workingDirectory = QDir::tempPath();

    QSignalSpy cancelledFinishedSpy(&controller, &AlifRunController::finished);
    QVERIFY(controller.start(longRequest, &startError));
    QTRY_VERIFY_WITH_TIMEOUT(controller.isActive(), 1000);
    controller.cancel();
    QTRY_VERIFY_WITH_TIMEOUT(cancelledFinishedSpy.count() == 1, 4000);
    QVERIFY(!controller.isActive());
}

void DockableToolsTest::recoveryStoreAtomicallyPersistsReadsAndRemovesSnapshots()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    RecoveryStore store(temporaryDirectory.filePath(QStringLiteral("recovery")));

    RecoveryEntry entry;
    entry.id = QStringLiteral("recovery-test-01");
    entry.displayName = QStringLiteral("اختبار.alif");
    entry.documentRevision = 7;
    entry.untitled = true;
    QVERIFY(store.writeSnapshot(entry, QStringLiteral("اطبع(\"استعادة\")")));

    const QVector<RecoveryEntry> entries = store.entries();
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().id, entry.id);
    QCOMPARE(entries.first().documentRevision, quint64(7));

    QString recoveredText;
    QVERIFY(store.readSnapshot(entries.first(), &recoveredText));
    QCOMPARE(recoveredText, QStringLiteral("اطبع(\"استعادة\")"));
    QVERIFY(store.removeEntry(entry.id));
    QVERIFY(store.entries().isEmpty());

    RecoveryEntry unsafeEntry;
    unsafeEntry.id = QStringLiteral("../outside-recovery-root");
    unsafeEntry.untitled = true;
    QVERIFY(!store.writeSnapshot(unsafeEntry, QStringLiteral("لا يجب حفظ هذه النسخة")));
}

void DockableToolsTest::recoveryCoordinatorReportsRemovalFailureAndAsynchronousFlushOutcome()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString recoveryRoot = temporaryDirectory.filePath(QStringLiteral("recovery"));
    QVERIFY(QDir().mkpath(recoveryRoot));

    const QString entryId = QStringLiteral("recovery-remove-failure-01");
    // A directory at the metadata-file path deterministically makes QFile::remove()
    // fail on every supported desktop platform.
    QVERIFY(QDir().mkpath(QDir(recoveryRoot).filePath(entryId + QStringLiteral(".json"))));

    RecoveryCoordinator coordinator(recoveryRoot);
    QSignalSpy removalFailureSpy(&coordinator, &RecoveryCoordinator::removalFailed);
    QSignalSpy flushSpy(&coordinator, &RecoveryCoordinator::flushCompleted);

    bool queuedCallbackRan = false;
    QTimer::singleShot(0, &coordinator, [&queuedCallbackRan]() { queuedCallbackRan = true; });
    coordinator.removeEntry(entryId);
    coordinator.requestFlush(500);

    // requestFlush() must return without nested event processing or re-entry.
    QVERIFY(!queuedCallbackRan);
    QVERIFY(flushSpy.isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(removalFailureSpy.count(), 1, 2000);
    QTRY_COMPARE_WITH_TIMEOUT(flushSpy.count(), 1, 2000);
    QCOMPARE(flushSpy.constFirst().constFirst().toBool(), false);
    QVERIFY(!removalFailureSpy.constFirst().at(1).toString().isEmpty());
}

void DockableToolsTest::editorRecoveryClearsDirtyOnlyAfterLatestSnapshotAcknowledgement()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    RecoveryCoordinator coordinator(temporaryDirectory.filePath(QStringLiteral("recovery")));

    TEditor editor;
    EditorPreferences preferences;
    preferences.autoSaveEnabled = true;
    editor.applyPreferences(preferences);
    editor.setRecoveryCoordinator(&coordinator);

    QSignalSpy persistedSpy(&coordinator, &RecoveryCoordinator::snapshotPersisted);
    editor.setPlainText(QStringLiteral("اطبع(\"لقطة مؤكدة\")"));
    editor.flushRecoverySnapshot();

    QVERIFY(editor.hasPendingRecoveryPersistence());
    QVERIFY(editor.lastRequestedRecoveryRevision() > 0);
    QCOMPARE(editor.lastPersistedRecoveryRevision(), quint64(0));
    QTRY_COMPARE_WITH_TIMEOUT(persistedSpy.count(), 1, 3000);
    QVERIFY(persistedSpy.constFirst().constFirst().value<RecoveryWriteResult>().succeeded);
    QTRY_VERIFY_WITH_TIMEOUT(!editor.hasPendingRecoveryPersistence(), 1000);
    QCOMPARE(editor.lastPersistedRecoveryRevision(), editor.lastRequestedRecoveryRevision());
    QCOMPARE(editor.lastPersistedRecoveryRevision(), editor.currentDirtyRecoveryRevision());
}

void DockableToolsTest::editorRecoveryFailureKeepsDirtyAndSchedulesBoundedRetry()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString blockedRecoveryRoot = temporaryDirectory.filePath(QStringLiteral("blocked-recovery"));
    QFile rootBlocker(blockedRecoveryRoot);
    QVERIFY(rootBlocker.open(QIODevice::WriteOnly));
    rootBlocker.close();

    RecoveryCoordinator coordinator(blockedRecoveryRoot);
    TEditor editor;
    EditorPreferences preferences;
    preferences.autoSaveEnabled = true;
    editor.applyPreferences(preferences);
    editor.setRecoveryCoordinator(&coordinator);

    QSignalSpy persistedSpy(&coordinator, &RecoveryCoordinator::snapshotPersisted);
    editor.setPlainText(QStringLiteral("اطبع(\"لقطة ستفشل\")"));
    editor.flushRecoverySnapshot();

    QTRY_COMPARE_WITH_TIMEOUT(persistedSpy.count(), 1, 3000);
    const RecoveryWriteResult result =
        persistedSpy.constFirst().constFirst().value<RecoveryWriteResult>();
    QVERIFY(!result.succeeded);
    QTRY_VERIFY_WITH_TIMEOUT(editor.hasPendingRecoveryPersistence(), 1000);
    QVERIFY(editor.isRecoveryRetryScheduled());
    QCOMPARE(editor.lastPersistedRecoveryRevision(), quint64(0));
    QCOMPARE(editor.lastRequestedRecoveryRevision(), editor.currentDirtyRecoveryRevision());
}

void DockableToolsTest::recoveryDialogUsesRtlAndSelectsEntriesByDefault()
{
    RecoveryEntry entry;
    entry.id = QStringLiteral("recovery-test-02");
    entry.displayName = QStringLiteral("غير معنون");
    entry.documentRevision = 1;
    entry.untitled = true;
    entry.capturedAtUtc = QDateTime::currentDateTimeUtc();

    TRecoveryDialog dialog({entry});
    QCOMPARE(dialog.layoutDirection(), Qt::RightToLeft);
    QVERIFY(dialog.findChild<QTreeWidget*>(QStringLiteral("RecoveryEntryList")) != nullptr);
    QVERIFY(dialog.findChild<QPushButton*>(QStringLiteral("RecoveryRestoreButton")) != nullptr);
    QCOMPARE(dialog.selectedEntries().size(), 1);
    QCOMPARE(dialog.selectedEntries().first().id, entry.id);
}

void DockableToolsTest::settingsWindowUsesRtlDraftApplyCancelWorkflow()
{
    auto* const application = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(application != nullptr);
    ApplicationBootstrap::initialize(*application);

    TSettings settings;
    QCOMPARE(settings.layoutDirection(), Qt::RightToLeft);

    auto* const fontSizeSpin = settings.findChild<QSpinBox*>(QStringLiteral("SettingsFontSizeSpin"));
    auto* const applyButton = settings.findChild<QPushButton*>(QStringLiteral("SettingsApplyButton"));
    auto* const cancelButton = settings.findChild<QPushButton*>(QStringLiteral("SettingsCancelButton"));
    auto* const resetButton = settings.findChild<QPushButton*>(QStringLiteral("SettingsResetButton"));
    QVERIFY(fontSizeSpin != nullptr);
    QVERIFY(applyButton != nullptr);
    QVERIFY(cancelButton != nullptr);
    QVERIFY(resetButton != nullptr);
    QCOMPARE(applyButton->objectName(), QStringLiteral("SettingsApplyButton"));

    const int baselineSize = settings.currentPreferences().fontSize;
    QSignalSpy previewSpy(&settings, &TSettings::preferencesPreviewed);
    const int previewSize = baselineSize == 12 ? 13 : 12;
    fontSizeSpin->setValue(previewSize);
    QVERIFY(previewSpy.count() >= 1);
    QCOMPARE(settings.currentPreferences().fontSize, previewSize);

    cancelButton->click();
    QCOMPARE(settings.currentPreferences().fontSize, baselineSize);

    resetButton->click();
    QVERIFY(settings.currentPreferences() == PreferencesStore::defaults());
}

void DockableToolsTest::editorPreferencesNormalizeInvalidValues()
{
    EditorPreferences preferences = PreferencesStore::defaults();
    preferences.fontFamily = QStringLiteral("   ");
    preferences.fontSize = 1;
    preferences.syntaxThemeIndex = 99;
    preferences.tabWidth = 99;
    preferences.autoSaveIntervalMilliseconds = 1;
    preferences.hoverDelayMilliseconds = 9000;

    const EditorPreferences normalized = PreferencesStore::normalize(preferences);
    QCOMPARE(normalized.fontFamily, PreferencesStore::defaults().fontFamily);
    QCOMPARE(normalized.fontSize, 12);
    QCOMPARE(normalized.syntaxThemeIndex, 3);
    QCOMPARE(normalized.tabWidth, 8);
    QCOMPARE(normalized.autoSaveIntervalMilliseconds, 5000);
    QCOMPARE(normalized.hoverDelayMilliseconds, 1500);
}

void DockableToolsTest::applicationBootstrapProvidesStableFontRolesAndLaunchValidation()
{
    auto* const application = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(application != nullptr);

    ApplicationBootstrap::initialize(*application);
    QCOMPARE(QCoreApplication::organizationName(), QStringLiteral("Alif"));
    QCOMPARE(QCoreApplication::applicationName(), QStringLiteral("Taif"));
    QCOMPARE(application->layoutDirection(), Qt::RightToLeft);
    QVERIFY(!ApplicationBootstrap::uiArabicFamily().isEmpty());
    QVERIFY(!ApplicationBootstrap::displayArabicFamily().isEmpty());
    QVERIFY(!ApplicationBootstrap::codeMonospaceFamily().isEmpty());
    QVERIFY(!ApplicationBootstrap::availableEditorFontFamilies().isEmpty());

    const ApplicationLaunchRequest validRequest =
        ApplicationBootstrap::parseLaunchRequest({QStringLiteral("Taif"), QStringLiteral("example.alif")});
    QVERIFY(validRequest.isValid());
    QCOMPARE(validRequest.filePath, QStringLiteral("example.alif"));

    const ApplicationLaunchRequest invalidRequest = ApplicationBootstrap::parseLaunchRequest(
        {QStringLiteral("Taif"), QStringLiteral("first.alif"), QStringLiteral("second.alif")});
    QVERIFY(!invalidRequest.isValid());
    QVERIFY(!invalidRequest.errorMessage.isEmpty());
}

void DockableToolsTest::bottomToolsArePersistentAndTabified()
{
    QMainWindow window;

    auto* diagnostics = new QDockWidget(QStringLiteral("المشكلات"), &window);
    diagnostics->setObjectName(QStringLiteral("DiagnosticsDock"));
    window.addDockWidget(Qt::BottomDockWidgetArea, diagnostics);

    const DockableConsoleTool terminal = DockableConsoleToolFactory::create(
        &window, QStringLiteral("طرفية النظام (CMD)"),
        QStringLiteral("TerminalDock"), QStringLiteral("SystemTerminalConsole"), false);
    const DockableConsoleTool output = DockableConsoleToolFactory::create(
        &window, QStringLiteral("مخرجات ألف"),
        QStringLiteral("AlifOutputDock"), QStringLiteral("AlifOutputConsole"), false);

    QVERIFY(terminal.dock != nullptr);
    QVERIFY(terminal.console != nullptr);
    QVERIFY(output.dock != nullptr);
    QVERIFY(output.console != nullptr);

    window.tabifyDockWidget(diagnostics, output.dock);
    window.tabifyDockWidget(output.dock, terminal.dock);
    window.show();
    QTest::qWait(10);

    QCOMPARE(window.dockWidgetArea(terminal.dock), Qt::BottomDockWidgetArea);
    QCOMPARE(window.dockWidgetArea(output.dock), Qt::BottomDockWidgetArea);

    const QDockWidget::DockWidgetFeatures requiredFeatures =
        QDockWidget::DockWidgetMovable
        | QDockWidget::DockWidgetFloatable
        | QDockWidget::DockWidgetClosable;
    QCOMPARE(terminal.dock->features() & requiredFeatures, requiredFeatures);
    QCOMPARE(output.dock->features() & requiredFeatures, requiredFeatures);

    QCOMPARE(terminal.dock->widget(), static_cast<QWidget*>(terminal.console));
    QCOMPARE(output.dock->widget(), static_cast<QWidget*>(output.console));
    QCOMPARE(terminal.console->objectName(), QStringLiteral("SystemTerminalConsole"));
    QCOMPARE(output.console->objectName(), QStringLiteral("AlifOutputConsole"));

    const QList<QDockWidget*> tabifiedWithProblems = window.tabifiedDockWidgets(diagnostics);
    QVERIFY(tabifiedWithProblems.contains(terminal.dock));
    QVERIFY(tabifiedWithProblems.contains(output.dock));

    QWidget* const terminalConsole = terminal.dock->widget();
    terminal.dock->hide();
    QVERIFY(terminal.dock->isHidden());
    terminal.dock->show();
    QTest::qWait(10);
    QCOMPARE(terminal.dock->widget(), terminalConsole);

    QWidget* const outputConsole = output.dock->widget();
    output.dock->hide();
    QVERIFY(output.dock->isHidden());
    output.dock->show();
    QTest::qWait(10);
    QCOMPARE(output.dock->widget(), outputConsole);
}

void DockableToolsTest::activationSelectsRequestedBottomToolTab()
{
    QMainWindow window;

    auto* diagnostics = new QDockWidget(QStringLiteral("المشكلات"), &window);
    diagnostics->setObjectName(QStringLiteral("DiagnosticsDock"));
    window.addDockWidget(Qt::BottomDockWidgetArea, diagnostics);

    const DockableConsoleTool terminal = DockableConsoleToolFactory::create(
        &window, QStringLiteral("طرفية النظام (CMD)"),
        QStringLiteral("TerminalDock"), QStringLiteral("SystemTerminalConsole"), false);
    const DockableConsoleTool output = DockableConsoleToolFactory::create(
        &window, QStringLiteral("مخرجات ألف"),
        QStringLiteral("AlifOutputDock"), QStringLiteral("AlifOutputConsole"), false);

    DockableConsoleToolFactory::ensureTabifiedWith(&window, diagnostics, output.dock);
    DockableConsoleToolFactory::ensureTabifiedWith(&window, diagnostics, terminal.dock);
    terminal.dock->hide();
    output.dock->hide();
    window.show();
    QTest::qWait(10);

    DockableConsoleToolFactory::showAndActivate(output.dock);
    QTRY_VERIFY(DockableConsoleToolFactory::isRenderedTab(output.dock));
    QVERIFY(!DockableConsoleToolFactory::isRenderedTab(terminal.dock));
    QVERIFY(window.tabifiedDockWidgets(diagnostics).contains(output.dock));

    DockableConsoleToolFactory::showAndActivate(terminal.dock);
    QTRY_VERIFY(DockableConsoleToolFactory::isRenderedTab(terminal.dock));
    QTRY_VERIFY(!DockableConsoleToolFactory::isRenderedTab(output.dock));
    QVERIFY(window.tabifiedDockWidgets(diagnostics).contains(terminal.dock));
}

void DockableToolsTest::viewMenuExposesOrderedDockActions()
{
    TMenuBar menuBar;

    QMenu* viewMenu = nullptr;
    for (QAction* const menuAction : menuBar.actions()) {
        if (menuAction->text() == QStringLiteral("عرض")) {
            viewMenu = menuAction->menu();
            break;
        }
    }

    QVERIFY(viewMenu != nullptr);
    QCOMPARE(viewMenu->layoutDirection(), Qt::RightToLeft);
    const QList<QAction*> viewActions = viewMenu->actions();
    QCOMPARE(viewActions.size(), 3);
    QCOMPARE(viewActions.at(0), menuBar.alifOutputAction);
    QCOMPARE(viewActions.at(1), menuBar.terminalAction);
    QCOMPARE(viewActions.at(2), menuBar.problemsAction);
    QCOMPARE(menuBar.alifOutputAction->text(), QStringLiteral("مخرجات ألف"));
    QCOMPARE(menuBar.terminalAction->text(), QStringLiteral("الطرفية"));
    QCOMPARE(menuBar.problemsAction->text(), QStringLiteral("الأخطاء"));
    QVERIFY(menuBar.alifOutputAction->isCheckable());
    QVERIFY(menuBar.terminalAction->isCheckable());
    QVERIFY(menuBar.problemsAction->isCheckable());

    QSignalSpy alifOutputSpy(&menuBar, &TMenuBar::showAlifOutputRequested);
    QSignalSpy terminalSpy(&menuBar, &TMenuBar::showTerminalRequested);
    QSignalSpy problemsSpy(&menuBar, &TMenuBar::showProblemsRequested);

    menuBar.alifOutputAction->trigger();
    QVERIFY(menuBar.alifOutputAction->isChecked());
    QVERIFY(!menuBar.terminalAction->isChecked());
    QVERIFY(!menuBar.problemsAction->isChecked());

    menuBar.terminalAction->trigger();
    QVERIFY(menuBar.alifOutputAction->isChecked());
    QVERIFY(menuBar.terminalAction->isChecked());
    QVERIFY(!menuBar.problemsAction->isChecked());

    menuBar.problemsAction->trigger();
    QVERIFY(menuBar.alifOutputAction->isChecked());
    QVERIFY(menuBar.terminalAction->isChecked());
    QVERIFY(menuBar.problemsAction->isChecked());

    menuBar.setOpenViewToolActions(false, true, true);
    QVERIFY(!menuBar.alifOutputAction->isChecked());
    QVERIFY(menuBar.terminalAction->isChecked());
    QVERIFY(menuBar.problemsAction->isChecked());

    QCOMPARE(alifOutputSpy.count(), 1);
    QCOMPARE(terminalSpy.count(), 1);
    QCOMPARE(problemsSpy.count(), 1);
}

void DockableToolsTest::editMenuExposesOrderedCommandActions()
{
    TMenuBar menuBar;

    QMenu* editMenu = nullptr;
    for (QAction* const menuAction : menuBar.actions()) {
        if (menuAction->text() == QStringLiteral("تحرير")) {
            editMenu = menuAction->menu();
            break;
        }
    }

    QVERIFY(editMenu != nullptr);
    QCOMPARE(editMenu->layoutDirection(), Qt::RightToLeft);

    QList<QAction*> commandActions;
    for (QAction* const action : editMenu->actions()) {
        if (!action->isSeparator()) {
            commandActions.append(action);
        }
    }

    const QList<QAction*> expectedActions = {
        menuBar.undoAction, menuBar.redoAction,
        menuBar.cutAction, menuBar.copyAction, menuBar.pasteAction,
        menuBar.findAction, menuBar.replaceAction, menuBar.goToLineAction,
        menuBar.toggleCommentAction, menuBar.duplicateLineAction,
        menuBar.moveLineUpAction, menuBar.moveLineDownAction
    };
    QCOMPARE(commandActions, expectedActions);

    QCOMPARE(menuBar.undoAction->text(), QStringLiteral("تراجع"));
    QCOMPARE(menuBar.redoAction->text(), QStringLiteral("إعادة"));
    QCOMPARE(menuBar.replaceAction->text(), QStringLiteral("بحث واستبدال"));
    QCOMPARE(menuBar.undoAction->shortcut(), QKeySequence::Undo);
    QCOMPARE(menuBar.redoAction->shortcut(), QKeySequence::Redo);
    QCOMPARE(menuBar.findAction->shortcut(), QKeySequence::Find);
    QCOMPARE(menuBar.replaceAction->shortcut(), QKeySequence(QStringLiteral("Ctrl+H")));
    QCOMPARE(menuBar.goToLineAction->shortcut(), QKeySequence(QStringLiteral("Ctrl+G")));
    QCOMPARE(menuBar.toggleCommentAction->objectName(), QStringLiteral("ToggleCommentAction"));
    QCOMPARE(menuBar.moveLineDownAction->objectName(), QStringLiteral("MoveLineDownAction"));

    QSignalSpy undoSpy(&menuBar, &TMenuBar::undoRequested);
    QSignalSpy redoSpy(&menuBar, &TMenuBar::redoRequested);
    QSignalSpy cutSpy(&menuBar, &TMenuBar::cutRequested);
    QSignalSpy copySpy(&menuBar, &TMenuBar::copyRequested);
    QSignalSpy pasteSpy(&menuBar, &TMenuBar::pasteRequested);
    QSignalSpy findSpy(&menuBar, &TMenuBar::findRequested);
    QSignalSpy replaceSpy(&menuBar, &TMenuBar::replaceRequested);
    QSignalSpy goToLineSpy(&menuBar, &TMenuBar::goToLineRequested);
    QSignalSpy commentSpy(&menuBar, &TMenuBar::toggleCommentRequested);
    QSignalSpy duplicateSpy(&menuBar, &TMenuBar::duplicateLineRequested);
    QSignalSpy moveUpSpy(&menuBar, &TMenuBar::moveLineUpRequested);
    QSignalSpy moveDownSpy(&menuBar, &TMenuBar::moveLineDownRequested);

    for (QAction* const action : expectedActions) {
        action->trigger();
    }

    QCOMPARE(undoSpy.count(), 1);
    QCOMPARE(redoSpy.count(), 1);
    QCOMPARE(cutSpy.count(), 1);
    QCOMPARE(copySpy.count(), 1);
    QCOMPARE(pasteSpy.count(), 1);
    QCOMPARE(findSpy.count(), 1);
    QCOMPARE(replaceSpy.count(), 1);
    QCOMPARE(goToLineSpy.count(), 1);
    QCOMPARE(commentSpy.count(), 1);
    QCOMPARE(duplicateSpy.count(), 1);
    QCOMPARE(moveUpSpy.count(), 1);
    QCOMPARE(moveDownSpy.count(), 1);
}

void DockableToolsTest::searchPanelProvidesReplaceSurface()
{
    QWidget window;
    window.resize(900, 620);
    QPlainTextEdit editor(&window);
    editor.setGeometry(120, 50, 740, 520);
    window.show();

    SearchPanel panel(&window);
    panel.showIn(&editor);
    QTRY_VERIFY(panel.isVisible());

    QCOMPARE(panel.layoutDirection(), Qt::RightToLeft);
    QCOMPARE(panel.anchorWidget(), static_cast<QWidget*>(&editor));
    QCOMPARE(panel.parentWidget(), static_cast<QWidget*>(&window));
    QVERIFY(panel.windowFlags().testFlag(Qt::FramelessWindowHint));
    QVERIFY(panel.geometry().top() >= editor.geometry().top());
    QVERIFY(panel.geometry().right() < editor.geometry().right());
    auto* const searchInput = panel.findChild<QLineEdit*>(QStringLiteral("SearchInput"));
    auto* const replacementInput = panel.findChild<QLineEdit*>(QStringLiteral("ReplacementInput"));
    auto* const replaceRow = panel.findChild<QWidget*>(QStringLiteral("ReplaceRow"));
    auto* const nextButton = panel.findChild<QPushButton*>(QStringLiteral("FindNextButton"));
    auto* const previousButton = panel.findChild<QPushButton*>(QStringLiteral("FindPreviousButton"));
    auto* const replaceButton = panel.findChild<QPushButton*>(QStringLiteral("ReplaceOneButton"));
    auto* const replaceAllButton = panel.findChild<QPushButton*>(QStringLiteral("ReplaceAllButton"));
    auto* const matchInfo = panel.findChild<QLabel*>(QStringLiteral("SearchMatchInfo"));

    QVERIFY(searchInput != nullptr);
    QVERIFY(replacementInput != nullptr);
    QVERIFY(replaceRow != nullptr);
    QVERIFY(nextButton != nullptr);
    QVERIFY(previousButton != nullptr);
    QVERIFY(replaceButton != nullptr);
    QVERIFY(replaceAllButton != nullptr);
    QVERIFY(matchInfo != nullptr);

    panel.showReplaceRow(false);
    QVERIFY(!replaceRow->isVisible());
    QCOMPARE(panel.height(), 48);
    panel.showReplaceRow(true);
    QTRY_VERIFY(replaceRow->isVisible());
    QCOMPARE(panel.height(), 96);

    editor.resize(620, 520);
    QTRY_VERIFY(panel.geometry().right() < editor.geometry().right());

    QSignalSpy findTextSpy(&panel, &SearchPanel::findText);
    QSignalSpy nextSpy(&panel, &SearchPanel::findNext);
    QSignalSpy previousSpy(&panel, &SearchPanel::findPrevious);
    QSignalSpy replaceOneSpy(&panel, &SearchPanel::replaceOne);
    QSignalSpy replaceAllSpy(&panel, &SearchPanel::replaceAll);

    searchInput->setText(QStringLiteral("نص"));
    replacementInput->setText(QStringLiteral("بديل"));
    QCOMPARE(panel.searchText(), QStringLiteral("نص"));
    QCOMPARE(panel.replaceText(), QStringLiteral("بديل"));
    QCOMPARE(findTextSpy.count(), 1);

    nextButton->click();
    previousButton->click();
    replaceButton->click();
    replaceAllButton->click();
    QCOMPARE(nextSpy.count(), 1);
    QCOMPARE(previousSpy.count(), 1);
    QCOMPARE(replaceOneSpy.count(), 1);
    QCOMPARE(replaceAllSpy.count(), 1);

    panel.setMatchInfo(2, 5);
    QCOMPARE(matchInfo->text(), QStringLiteral("2/5"));
    panel.setNoMatchesFound(true);
    QVERIFY(searchInput->styleSheet().contains(QStringLiteral("#ef4444")));
}

void DockableToolsTest::searchReplaceEnginePreservesMatchAndUndoSemantics()
{
    const SearchReplaceEngine::Query wholeWordQuery{
        QStringLiteral("سيارة"), QStringLiteral("مركبة"), false, true, false};
    const QString originalText = QStringLiteral("سيارة سيارة سيارات");
    const QList<SearchReplaceEngine::MatchRange> matches =
        SearchReplaceEngine::collectMatches(originalText, wholeWordQuery);
    QCOMPARE(matches.size(), 2);

    QTextDocument document(originalText);
    SearchReplaceEngine::replaceAll(&document, originalText, matches, wholeWordQuery);
    QCOMPARE(document.toPlainText(), QStringLiteral("مركبة مركبة سيارات"));
    document.undo();
    QCOMPARE(document.toPlainText(), originalText);

    const SearchReplaceEngine::Query regexQuery{
        QStringLiteral("لون[0-9]"), QStringLiteral("خاصية"), true, false, true};
    const QString regexText = QStringLiteral("لون1 لون2 لون");
    QCOMPARE(SearchReplaceEngine::collectMatches(regexText, regexQuery).size(), 2);

    const SearchReplaceEngine::Query invalidRegexQuery{
        QStringLiteral("["), QStringLiteral("بديل"), false, false, true};
    QVERIFY(!SearchReplaceEngine::isValid(invalidRegexQuery));
    QVERIFY(SearchReplaceEngine::collectMatches(regexText, invalidRegexQuery).isEmpty());
}

void DockableToolsTest::sessionStorePersistsNormalizedSessions()
{
    const QString settingsFile = QDir(QDir::tempPath()).filePath(
        QStringLiteral("taif-session-%1.ini")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    const SessionStore::SettingsScope scope{
        QStringLiteral("TaifEditorSessionStoreTests"),
        QStringLiteral("SessionScope"), settingsFile};
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.clear();

    SessionStore store(scope);
    SavedSession session;
    session.displayName = QStringLiteral("  مشروع السيارة  ");
    session.filePaths = {
        QDir::tempPath() + QStringLiteral("/taif-session-one.alif"),
        QDir::tempPath() + QStringLiteral("/taif-session-one.alif"),
        QDir::tempPath() + QStringLiteral("/taif-session-two.alif")};
    session.activeFilePath = session.filePaths.first();

    QString errorMessage;
    QVERIFY(store.create(session, &errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const QVector<SavedSession> sessions = store.loadAll();
    QCOMPARE(sessions.size(), 1);
    QCOMPARE(sessions.first().displayName, QStringLiteral("مشروع السيارة"));
    QCOMPARE(sessions.first().filePaths.size(), 2);
    QVERIFY(!sessions.first().id.isEmpty());
    QCOMPARE(sessions.first().activeFilePath, SessionStore::normalizePath(session.activeFilePath));

    QVERIFY(!store.create(session, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    SavedSession updated = sessions.first();
    updated.displayName = QStringLiteral("مشروع محدث");
    std::swap(updated.filePaths[0], updated.filePaths[1]);
    QVERIFY(store.update(updated, &errorMessage));
    const QVector<SavedSession> updatedSessions = store.loadAll();
    QCOMPARE(updatedSessions.first().displayName, QStringLiteral("مشروع محدث"));
    QCOMPARE(updatedSessions.first().filePaths, updated.filePaths);

    QVERIFY(store.remove(updated.id, &errorMessage));
    QVERIFY(store.loadAll().isEmpty());
    QFile::remove(settingsFile);
}

void DockableToolsTest::breadcrumbBarUsesRtlAndRepresentsUntitledFile()
{
    TBreadcrumbBar bar;
    QCOMPARE(bar.layoutDirection(), Qt::RightToLeft);

    bar.setFileContext({});
    const QList<QToolButton*> buttons = bar.findChildren<QToolButton*>();
    QCOMPARE(buttons.size(), 1);
    QCOMPARE(buttons.first()->text(), QStringLiteral("بدون عنوان"));
    QCOMPARE(buttons.first()->accessibleName(), QStringLiteral("ملف: بدون عنوان"));
}

void DockableToolsTest::breadcrumbBarRendersOrderedFileAndSemanticSegments()
{
    TBreadcrumbBar bar;
    bar.setFileContext(QStringLiteral("C:/workspace/مشروع/سيارة.alif"));

    EditorBreadcrumbContext context;
    context.revision = 42;
    context.cursorOffset = 91;
    context.symbolPath = {
        SemanticBreadcrumb{17, SymbolKind::Class, QStringLiteral("سيارة"),
                           {{10, 1, 1}, {15, 1, 6}}, {{10, 1, 1}, {160, 12, 1}}},
        SemanticBreadcrumb{18, SymbolKind::Function, QStringLiteral("تغيير_اللون"),
                           {{35, 3, 1}, {47, 3, 13}}, {{35, 3, 1}, {100, 8, 1}}}};
    bar.setSemanticContext(context);

    const QList<QToolButton*> buttons = bar.findChildren<QToolButton*>();
    QCOMPARE(buttons.size(), 4);
    QCOMPARE(buttons.at(0)->text(), QStringLiteral("مشروع"));
    QCOMPARE(buttons.at(1)->text(), QStringLiteral("سيارة.alif"));
    QCOMPARE(buttons.at(2)->text(), QStringLiteral("صنف سيارة"));
    QCOMPARE(buttons.at(3)->text(), QStringLiteral("دالة تغيير_اللون"));
    QCOMPARE(buttons.at(0)->layoutDirection(), Qt::RightToLeft);
    QCOMPARE(buttons.at(1)->layoutDirection(), Qt::RightToLeft);

    bar.resize(900, 36);
    bar.show();
    QTest::qWait(10);
    QVERIFY(buttons.at(0)->geometry().center().x() > buttons.at(1)->geometry().center().x());
    QVERIFY(buttons.at(1)->geometry().center().x() > buttons.at(2)->geometry().center().x());
    QVERIFY(buttons.at(2)->geometry().center().x() > buttons.at(3)->geometry().center().x());
}

void DockableToolsTest::breadcrumbBarClearsSemanticSegmentsAndEmitsNavigationSignals()
{
    const QString folderPath = QDir(QDir::tempPath()).filePath(
        QStringLiteral("taif-breadcrumb-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QVERIFY(QDir().mkpath(folderPath));
    const QString filePath = QDir(folderPath).filePath(QStringLiteral("سيارة.alif"));

    TBreadcrumbBar bar;
    bar.setFileContext(filePath);

    SourceRange expectedRange{{22, 4, 1}, {34, 4, 13}};
    EditorBreadcrumbContext context;
    context.symbolPath = {
        SemanticBreadcrumb{17, SymbolKind::Function, QStringLiteral("تغيير_اللون"),
                           expectedRange, {{22, 4, 1}, {80, 7, 1}}}};
    bar.setSemanticContext(context);

    bool fileActivated = false;
    bool symbolActivated = false;
    QString emittedFilePath;
    SourceRange emittedRange;
    connect(&bar, &TBreadcrumbBar::fileSegmentActivated, this,
            [&fileActivated, &emittedFilePath](const QString& path) {
                fileActivated = true;
                emittedFilePath = path;
            });
    connect(&bar, &TBreadcrumbBar::symbolSegmentActivated, this,
            [&symbolActivated, &emittedRange](const SourceRange& range) {
                symbolActivated = true;
                emittedRange = range;
            });

    QList<QToolButton*> buttons = bar.findChildren<QToolButton*>();
    QCOMPARE(buttons.size(), 3);
    buttons.at(1)->click();
    buttons.at(2)->click();

    QVERIFY(fileActivated);
    QCOMPARE(emittedFilePath, bar.currentFilePath());
    QVERIFY(symbolActivated);
    QCOMPARE(emittedRange.begin.offset, expectedRange.begin.offset);
    QCOMPARE(emittedRange.end.offset, expectedRange.end.offset);

    bar.clearSemanticContext();
    buttons = bar.findChildren<QToolButton*>();
    QCOMPARE(buttons.size(), 2);
    QCOMPARE(buttons.at(0)->text(), QFileInfo(folderPath).fileName());
    QCOMPARE(buttons.at(1)->text(), QStringLiteral("سيارة.alif"));
    QVERIFY(QDir(folderPath).removeRecursively());
}

void DockableToolsTest::sessionEditorPreservesOrderedFilesInRtl()
{
    SessionEditorDialog dialog;
    SavedSession session;
    session.displayName = QStringLiteral("جلسة اختبار");
    session.filePaths = {
        QDir::tempPath() + QStringLiteral("/first.alif"),
        QDir::tempPath() + QStringLiteral("/second.alif")};
    dialog.setSession(session);
    dialog.show();
    QTest::qWait(10);

    QCOMPARE(dialog.layoutDirection(), Qt::RightToLeft);
    auto* const nameInput = dialog.findChild<QLineEdit*>(QStringLiteral("SessionNameInput"));
    auto* const filesList = dialog.findChild<QListWidget*>(QStringLiteral("SessionFilesList"));
    auto* const moveUpButton = dialog.findChild<QPushButton*>(QStringLiteral("MoveSessionFileUpButton"));
    QVERIFY(nameInput != nullptr);
    QVERIFY(filesList != nullptr);
    QVERIFY(moveUpButton != nullptr);
    QCOMPARE(nameInput->text(), QStringLiteral("جلسة اختبار"));
    QCOMPARE(filesList->count(), 2);

    filesList->setCurrentRow(1);
    moveUpButton->click();
    const SavedSession reordered = dialog.session();
    QCOMPARE(reordered.filePaths.size(), 2);
    QCOMPARE(reordered.filePaths.first(), SessionStore::normalizePath(session.filePaths.at(1)));
}

QTEST_MAIN(DockableToolsTest)
#include "tst_DockableTools.moc"
