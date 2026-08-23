#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QStackedWidget>
#include <QtGui/QImage>
#include <QtGui/QTextDocument>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QUuid>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QApplication>
#include <QtCore/QProcess>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "DockableConsoleTool.h"
#include "OutputBuffer.h"
#include "TConsole.h"
#include "InlinePromptConsole.h"
#include "TerminalScreenModel.h"
#include "TerminalSessionController.h"
#include "TerminalView.h"
#include "VtStreamParser.h"
#include "TMenu.h"
#include "TSearchPanel.h"
#include "SearchReplaceEngine.h"
#include "SessionEditorDialog.h"
#include "SessionStore.h"
#include "TBreadcrumbBar.h"
#include "ApplicationBootstrap.h"
#include "ApplicationWindowController.h"
#include "TWelcomeWindow.h"
#include "Taif.h"
#include "EditorPreferences.h"
#include "TSettings.h"
#include "RecoveryStore.h"
#include "RecoveryCoordinator.h"
#include "EditorAnalysisBinding.h"
#include "EditorRecoveryBinding.h"
#include "EditorInteractionBinding.h"
#include "TRecoveryDialog.h"
#include "TEditor.h"
#include "AiChatPanel.h"
#include "AiAssistantSettings.h"
#include "AiLineDiff.h"
#include "AiTextPatch.h"
#include "AiPatchReviewWidget.h"
#include "AiWorkspacePolicy.h"
#include "AiAgentController.h"
#include "LmStudioClient.h"
#include "interaction/MultiCursorController.h"
#include "EditorInfoBar.h"
#include "ProjectExplorerWidget.h"
#include "ProjectFileOperations.h"
#include "ProjectFileProxyModel.h"
#include "GitStatusService.h"
#include "GitRepositoryService.h"
#include "GitPanelWidget.h"
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
    void sessionStoreMigratesLegacyDataAndRecoversFromBackup();
    void sessionEditorPreservesOrderedFilesInRtl();
    void breadcrumbBarUsesRtlAndRepresentsUntitledFile();
    void breadcrumbBarRendersOrderedFileAndSemanticSegments();
    void breadcrumbBarClearsSemanticSegmentsAndEmitsNavigationSignals();
    void editorInfoBarPresentsSnapshotAndAdaptsToWidth();
    void editorInfoBarEmitsDiagnosticsActivation();
    void mainWindowInformationBarTracksActiveEditor();
    void applicationBootstrapProvidesStableFontRolesAndLaunchValidation();
    void applicationWindowControllerOwnsTopLevelWindowRouting();
    void editorPreferencesNormalizeInvalidValues();
    void settingsWindowUsesRtlDraftApplyCancelWorkflow();
    void settingsRecentFileClearRemainsDraftLocalUntilApply();
    void recoveryStoreAtomicallyPersistsReadsAndRemovesSnapshots();
    void recoveryCoordinatorReportsRemovalFailureAndAsynchronousFlushOutcome();
    void editorRecoveryClearsDirtyOnlyAfterLatestSnapshotAcknowledgement();
    void editorRecoveryFailureKeepsDirtyAndSchedulesBoundedRetry();
    void editorServiceBindingsHaveIdempotentShutdown();
    void recoveryDialogUsesRtlAndSelectsEntriesByDefault();
    void alifRunControllerValidatesRunsAndCancels();
    void outputBufferBoundsPendingChunksAndReportsTruncation();
    void consoleAppendsSplitCrLfAndBoundsRenderedOutput();
    void vtParserHandlesSplitSequencesAndAlternateScreen();
    void terminalViewRetainsAndSelectsScrolledHistory();
    void terminalSessionControllerCompletesOneShotCommand();
    void inlinePromptProtectsTranscriptAndPreservesHistory();
    void nativeTerminalDockActivationFocusesViewport();
    void projectFileOperationsRemainRootContained();
    void projectExplorerUsesRtlFilteringAndSafeRoot();
    void gitStatusServiceReportsReadOnlyFileStates();
    void mainWindowBindsProjectExplorerToFolderRoot();
    void gitRepositoryServiceRefreshesAndStagesWorktreeFile();
    void projectExplorerManualRefreshUpdatesGitStatus();
    void mainWindowExposesRightSideGitPanel();
    void gitPanelUsesDarkNavySurfaces();
    void workspaceAutoCommandPolicyFailsClosed();
    void aiLineDiffAlignsArabicAndChangedRowsDeterministically();
    void aiTextPatchAppliesAnchoredLineEditsAndRejectsWholeFileOrStaleRanges();
    void aiTextPatchPreservesCrLfAndFinalNewline();
    void aiAssistantSettingsUseWorkspaceAutoAndBoundLongTimeouts();
    void lmStudioClientSerializesAssistantToolCallsForContinuation();
    void workspaceAutoCompletesSafeReadAndContinues();
    void workspaceAutoProtectsUnsavedOpenFile();
    void aiPatchStagesForReviewAndWritesOnlyAfterAcceptance();
    void aiPatchReviewWidgetPresentsReadOnlyLtrSplitAndAcceptAction();
    void aiPatchPreviewAppearsAndUpdatesFromStreamedArguments();
    void aiPatchReviewWidgetStreamsProposedTokensBeforeEnablingAcceptance();
    void mainWindowSwitchesToAiPatchReviewWorkspace();
    void aiAgentControllerConstructsAndDestroys();
    void aiChatPanelConstructsAndDestroys();
    void aiPanelPresentsWorkspaceAutoControlsAndHidesRawToolPayload();
    void aiChatPanelUsesLeftDockRtlSurface();
    void multiCursorSelectsOccurrencesAndEditsInOneUndoStep();
    void multiCursorAddsVerticalCaretAndPreservesDuplicateLineCommand();
    void multiCursorControllerRetainsPrimaryAndDeduplicatesOverlaps();
    void multiCursorSupportsAltClickDeletionAndIndentedNewlines();
};

void DockableToolsTest::outputBufferBoundsPendingChunksAndReportsTruncation()
{
    OutputBuffer::Limits limits;
    limits.maximumPendingBytes = 64;
    limits.maximumChunkBytes = 32;
    OutputBuffer buffer(limits);

    buffer.append(QString(16, QLatin1Char('A')));
    buffer.append(QString(16, QLatin1Char('B')));
    buffer.append(QString(16, QLatin1Char('C')));

    const OutputBuffer::DrainResult result = buffer.drain();
    QVERIFY(result.truncated);
    QCOMPARE(result.droppedBytes, qsizetype(32));
    QCOMPARE(result.text, QString(16, QLatin1Char('B')) + QString(16, QLatin1Char('C')));
    QCOMPARE(buffer.pendingBytes(), qsizetype(0));
}

void DockableToolsTest::consoleAppendsSplitCrLfAndBoundsRenderedOutput()
{
    InlinePromptConsole console;
    console.show();
    auto* const output = static_cast<QPlainTextEdit*>(&console);

    console.appendPlainTextThreadSafe(QStringLiteral("مرحلة أولى\r"));
    console.appendPlainTextThreadSafe(QStringLiteral("\nمرحلة ثانية\n"));
    QTRY_VERIFY_WITH_TIMEOUT(output->toPlainText().contains(QStringLiteral("مرحلة أولى\nمرحلة ثانية")),
                             1000);

    console.appendPlainTextThreadSafe(QStringLiteral("10%\r"));
    console.appendPlainTextThreadSafe(QStringLiteral("20%"));
    QTRY_VERIFY_WITH_TIMEOUT(output->toPlainText().contains(QStringLiteral("20%")), 1000);
    QVERIFY(!output->toPlainText().contains(QStringLiteral("10%")));

    QString burst;
    for (int index = 0; index < TConsole::kMaximumRenderedLines + 300; ++index) {
        burst += QStringLiteral("سطر %1\n").arg(index);
    }
    console.appendPlainTextThreadSafe(burst);
    QTRY_VERIFY_WITH_TIMEOUT(output->toPlainText().contains(QStringLiteral("سطر %1")
                                                                 .arg(TConsole::kMaximumRenderedLines + 299)),
                             3000);
    QVERIFY(console.renderedLineCount() <= TConsole::kMaximumRenderedLines);

    console.appendPlainTextThreadSafe(QString(TConsole::kMaximumRenderedCharacters + 2048,
                                               QChar(0x0633)));
    QTRY_VERIFY_WITH_TIMEOUT(output->toPlainText().contains(QString(128, QChar(0x0633))), 3000);
    QVERIFY(console.renderedCharacterCount() <= TConsole::kMaximumRenderedCharacters + 128);
}

void DockableToolsTest::vtParserHandlesSplitSequencesAndAlternateScreen()
{
    TerminalScreenModel screen(12, 3);
    VtStreamParser parser(screen);

    parser.feed("A\x1b[3");
    parser.feed("1mB");
    QCOMPARE(screen.grid().at(0).at(0).text, QStringLiteral("A"));
    QCOMPARE(screen.grid().at(0).at(1).text, QStringLiteral("B"));
    QVERIFY(screen.grid().at(0).at(1).attributes.foreground.isValid());

    parser.feed("\x1b[2DZ");
    QCOMPARE(screen.grid().at(0).at(0).text, QStringLiteral("Z"));
    parser.feed("\x1b]2;Taif Native Terminal\x07");
    QCOMPARE(screen.title(), QStringLiteral("Taif Native Terminal"));

    parser.feed("\x1b[?1049hALT");
    QVERIFY(screen.usingAlternateScreen());
    QCOMPARE(screen.grid().at(0).at(0).text, QStringLiteral("A"));
    QCOMPARE(screen.grid().at(0).at(1).text, QStringLiteral("L"));
    parser.feed("\x1b[?1049l");
    QVERIFY(!screen.usingAlternateScreen());
    QCOMPARE(screen.grid().at(0).at(0).text, QStringLiteral("Z"));
    QCOMPARE(parser.ignoredSequenceCount(), 0);
}

void DockableToolsTest::terminalViewRetainsAndSelectsScrolledHistory()
{
    TerminalView view;
    view.resize(220, 64);
    view.show();
    QTest::qWait(20);

    QByteArray transcript;
    for (int index = 0; index < 16; ++index) {
        transcript += QByteArrayLiteral("history-") + QByteArray::number(index) + QByteArrayLiteral("\r\n");
    }
    view.appendOutput(transcript);
    QVERIFY(view.screen().scrollback().size() > 0);

    QScrollBar* const scrollBar = view.verticalScrollBar();
    QVERIFY(scrollBar->maximum() > 0);
    scrollBar->setValue(scrollBar->minimum());
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(2, 8));
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(152, 8));
    QVERIFY2(view.selectedText().startsWith(QStringLiteral("history-0")),
             qPrintable(view.selectedText()));

    scrollBar->setValue(scrollBar->maximum());
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(2, 8));
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(152, 8));
    QVERIFY2(view.selectedText().startsWith(QStringLiteral("history-")),
             qPrintable(view.selectedText()));
}

void DockableToolsTest::terminalSessionControllerCompletesOneShotCommand()
{
    TerminalSessionController controller;
    QSignalSpy stateSpy(&controller, &TerminalSessionController::stateChanged);
    QSignalSpy outputSpy(&controller, &TerminalSessionController::outputReady);
    QSignalSpy finishedSpy(&controller, &TerminalSessionController::finished);

    TerminalSessionController::Request request;
    request.workingDirectory = QDir::tempPath();
    request.initialGrid = QSize(80, 24);

#if defined(Q_OS_WIN)
    request.program = qEnvironmentVariable("COMSPEC", QStringLiteral("C:\\Windows\\System32\\cmd.exe"));
    request.arguments = {QStringLiteral("/Q"), QStringLiteral("/K")};
    const QByteArray expectedOutput = QDir::toNativeSeparators(QDir::tempPath()).toLocal8Bit();
    const QByteArray command = QByteArrayLiteral("cd\r");
#elif defined(Q_OS_UNIX)
    request.program = QStringLiteral("/bin/sh");
    request.arguments = {QStringLiteral("-i")};
    const QByteArray expectedOutput = QByteArrayLiteral("TAIF_POSIX_PTY_READY");
    const QByteArray command = QByteArrayLiteral("printf 'TAIF_POSIX_PTY_READY\\n'\rexit\r");
#else
    QSKIP("No native terminal transport is available on this platform.");
#endif

    QString error;
    QVERIFY2(controller.start(request, &error), qPrintable(error));
    QTRY_COMPARE_WITH_TIMEOUT(controller.state(), TerminalSessionController::State::Running, 1000);
    controller.sendInput(command);

    QByteArray capturedOutput;
    QTRY_VERIFY_WITH_TIMEOUT(([&outputSpy, &capturedOutput, &expectedOutput]() {
        for (const QList<QVariant>& arguments : std::as_const(outputSpy)) {
            capturedOutput.append(arguments.constFirst().toByteArray());
        }
        outputSpy.clear();
        return capturedOutput.contains(expectedOutput);
    })(), 5000);
    QVERIFY2(capturedOutput.contains(expectedOutput),
             qPrintable(QStringLiteral("Captured terminal bytes: %1")
                            .arg(QString::fromUtf8(capturedOutput).toHtmlEscaped())));

#if defined(Q_OS_WIN)
    controller.shutdown();
#endif
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3000);
    QVERIFY(!controller.isActive());
    QVERIFY(stateSpy.count() >= 3);
}

void DockableToolsTest::nativeTerminalDockActivationFocusesViewport()
{
    QTemporaryDir projectDirectory;
    QVERIFY(projectDirectory.isValid());

    QMainWindow host;
    host.resize(900, 600);
    host.show();

    DockableConsoleTool terminal = DockableConsoleToolFactory::create(
        &host, QStringLiteral("اختبار الطرفية"), QStringLiteral("TerminalFocusDock"),
        QStringLiteral("TerminalFocusConsole"), true);
    QVERIFY(terminal.dock != nullptr);
    QVERIFY(terminal.console != nullptr);
    QVERIFY(terminal.console->isNativeTerminal());

    auto* const view = terminal.console->findChild<TerminalView*>();
    QVERIFY(view != nullptr);
    terminal.console->setTerminalWorkingDirectory(projectDirectory.path());
    QCOMPARE(terminal.console->terminalWorkingDirectory(), QDir(projectDirectory.path()).absolutePath());
    DockableConsoleToolFactory::showAndActivate(terminal.dock);
    QTRY_VERIFY_WITH_TIMEOUT(view->isVisible(), 1000);
    QTRY_COMPARE_WITH_TIMEOUT(QApplication::focusWidget(), static_cast<QWidget*>(view), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(view->gridSize().height() > 1, 1000);
    const QImage beforeRender = view->grab().toImage();
    terminal.console->appendPlainTextThreadSafe(QStringLiteral("terminal-grid-rendered"));
    QTRY_VERIFY_WITH_TIMEOUT(view->screen().text().contains(QStringLiteral("terminal-grid-rendered")),
                             1000);
    QTRY_VERIFY_WITH_TIMEOUT(([view, &beforeRender]() {
        const QImage afterRender = view->grab().toImage();
        if (afterRender.size() != beforeRender.size()) {
            return false;
        }
        int changedPixels = 0;
        for (int y = 0; y < afterRender.height(); ++y) {
            for (int x = 0; x < afterRender.width(); ++x) {
                if (afterRender.pixelColor(x, y) != beforeRender.pixelColor(x, y)
                    && ++changedPixels >= 20) {
                    return true;
                }
            }
        }
        return false;
    })(), 1000);

    terminal.console->stopCmd();
}

void DockableToolsTest::inlinePromptProtectsTranscriptAndPreservesHistory()
{
    InlinePromptConsole console;
    console.show();
    console.setFocus(Qt::OtherFocusReason);
    console.beginInput();
    QVERIFY(console.acceptsInput());

    QTest::keyClicks(&console, QStringLiteral("draft"));
    console.appendPlainTextThreadSafe(QStringLiteral("نتيجة التنفيذ\n"));
    QTRY_VERIFY_WITH_TIMEOUT(console.toPlainText().contains(
                                 QStringLiteral("نتيجة التنفيذ\nألف › draft")), 1000);

    QTextCursor transcriptCursor(console.document());
    transcriptCursor.setPosition(0);
    console.setTextCursor(transcriptCursor);
    QTest::keyClick(&console, Qt::Key_Backspace);
    QVERIFY(console.toPlainText().contains(QStringLiteral("نتيجة التنفيذ")));
    QVERIFY(console.toPlainText().endsWith(QStringLiteral("ألف › draft")));

    QSignalSpy commandSpy(&console, &TConsole::commandEntered);
    QTest::keyClick(&console, Qt::Key_Return);
    QTRY_COMPARE_WITH_TIMEOUT(commandSpy.count(), 1, 1000);
    QCOMPARE(commandSpy.constFirst().constFirst().toString(), QStringLiteral("draft"));

    QTest::keyClick(&console, Qt::Key_Up);
    QVERIFY(console.toPlainText().endsWith(QStringLiteral("ألف › draft")));
}

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

void DockableToolsTest::editorInfoBarPresentsSnapshotAndAdaptsToWidth()
{
    EditorInfoBar infoBar;
    infoBar.resize(1240, 34);
    infoBar.show();

    EditorInfoSnapshot snapshot;
    snapshot.hasEditor = true;
    snapshot.documentName = QStringLiteral("حالة.alif");
    snapshot.documentPath = QStringLiteral("C:/work/حالة.alif");
    snapshot.modified = true;
    snapshot.line = 12;
    snapshot.column = 7;
    snapshot.selectedCharacters = 18;
    snapshot.selectedLines = 2;
    snapshot.documentLines = 250;
    snapshot.documentCharacters = 9200;
    snapshot.lineEnding = EditorInfoSnapshot::LineEnding::Crlf;
    snapshot.indentationWidth = 8;
    snapshot.errorCount = 2;
    snapshot.warningCount = 1;
    snapshot.analysisState = EditorInfoSnapshot::AnalysisState::Ready;
    snapshot.analysisDurationMilliseconds = 14;
    snapshot.analysisSnapshotCharacters = 9200;
    snapshot.analysisTokenCount = 640;
    snapshot.recoveryState = EditorInfoSnapshot::RecoveryState::PendingPersistence;
    snapshot.recoveryWriteDurationMilliseconds = 5;
    infoBar.setSnapshot(snapshot);

    QCOMPARE(infoBar.layoutDirection(), Qt::RightToLeft);
    auto* const documentLabel = infoBar.findChild<QLabel*>(QStringLiteral("InfoDocumentSegmentLabel"));
    auto* const cursorLabel = infoBar.findChild<QLabel*>(QStringLiteral("InfoCursorSegmentLabel"));
    auto* const formatLabel = infoBar.findChild<QLabel*>(QStringLiteral("InfoFormatSegmentLabel"));
    auto* const diagnosticsButton = infoBar.findChild<QToolButton*>(QStringLiteral("InformationDiagnosticsButton"));
    QVERIFY(documentLabel != nullptr);
    QVERIFY(cursorLabel != nullptr);
    QVERIFY(formatLabel != nullptr);
    QVERIFY(diagnosticsButton != nullptr);
    QVERIFY(documentLabel->text().contains(QStringLiteral("حالة.alif")));
    QVERIFY(cursorLabel->text().contains(QStringLiteral("12")));
    QVERIFY(formatLabel->text().contains(QStringLiteral("CRLF")));
    QCOMPARE(cursorLabel->layoutDirection(), Qt::LeftToRight);
    QCOMPARE(formatLabel->layoutDirection(), Qt::LeftToRight);
    QVERIFY(diagnosticsButton->text().contains(QStringLiteral("2")));
    QVERIFY(!documentLabel->toolTip().isEmpty());

    infoBar.resize(720, 34);
    QTRY_VERIFY_WITH_TIMEOUT(!infoBar.findChild<QWidget*>(QStringLiteral("InfoDocumentSegment"))->isVisible(), 500);
    QVERIFY(!infoBar.findChild<QWidget*>(QStringLiteral("InfoAnalysisSegment"))->isVisible());
    QVERIFY(!infoBar.findChild<QWidget*>(QStringLiteral("InfoRecoverySegment"))->isVisible());
    QVERIFY(infoBar.findChild<QWidget*>(QStringLiteral("InfoDiagnosticsSegment"))->isVisible());
    QVERIFY(infoBar.findChild<QWidget*>(QStringLiteral("InfoCursorSegment"))->isVisible());
}

void DockableToolsTest::editorInfoBarEmitsDiagnosticsActivation()
{
    EditorInfoBar infoBar;
    infoBar.resize(900, 34);
    infoBar.show();
    QSignalSpy activationSpy(&infoBar, &EditorInfoBar::diagnosticsActivated);

    auto* const diagnosticsButton = infoBar.findChild<QToolButton*>(QStringLiteral("InformationDiagnosticsButton"));
    QVERIFY(diagnosticsButton != nullptr);
    QTest::mouseClick(diagnosticsButton, Qt::LeftButton);
    QCOMPARE(activationSpy.count(), 1);
}

void DockableToolsTest::mainWindowInformationBarTracksActiveEditor()
{
    Taif window({}, nullptr, true);
    window.resize(960, 680);
    window.show();

    auto* const infoBar = window.findChild<EditorInfoBar*>();
    auto* const editor = window.findChild<TEditor*>();
    QVERIFY(infoBar != nullptr);
    QVERIFY(editor != nullptr);

    editor->setPlainText(QStringLiteral("أول\nثانٍ"));
    QTextCursor cursor(editor->document());
    cursor.movePosition(QTextCursor::End);
    editor->setTextCursor(cursor);
    editor->insertPlainText(QStringLiteral("!"));

    QTRY_COMPARE_WITH_TIMEOUT(infoBar->snapshot().line, 2, 1000);
    QCOMPARE(infoBar->snapshot().column, 6);
    QCOMPARE(infoBar->snapshot().documentLines, qsizetype(2));
    QVERIFY(infoBar->snapshot().modified);
    QVERIFY(infoBar->findChild<QLabel*>(QStringLiteral("InfoCursorSegmentLabel"))->text().contains(
        QStringLiteral("2")));
}

void DockableToolsTest::projectFileOperationsRemainRootContained()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString root = temporaryDirectory.filePath(QStringLiteral("project"));
    const QString outside = temporaryDirectory.filePath(QStringLiteral("outside"));
    QVERIFY(QDir().mkpath(root));
    QVERIFY(QDir().mkpath(outside));

    const ProjectFileOperationResult created = ProjectFileOperations::createFile(root, root,
                                                                                   QStringLiteral("main.alif"));
    QVERIFY(created.succeeded);
    QVERIFY(QFileInfo::exists(created.destinationPath));
    QVERIFY(!ProjectFileOperations::createFile(root, root, QStringLiteral("../escape.alif")).succeeded);
    QVERIFY(!ProjectFileOperations::createFile(root, outside, QStringLiteral("outside.alif")).succeeded);

    const ProjectFileOperationResult folder = ProjectFileOperations::createFolder(root, root,
                                                                                    QStringLiteral("lib"));
    QVERIFY(folder.succeeded);
    const ProjectFileOperationResult renamed = ProjectFileOperations::renamePath(root, created.destinationPath,
                                                                                   QStringLiteral("status.alif"));
    QVERIFY(renamed.succeeded);
    QVERIFY(QFileInfo::exists(renamed.destinationPath));
    QVERIFY(!ProjectFileOperations::renamePath(root, root, QStringLiteral("renamed-root")).succeeded);
    QVERIFY(ProjectFileOperations::permanentlyDelete(root, renamed.destinationPath).succeeded);
    QVERIFY(!QFileInfo::exists(renamed.destinationPath));
}

void DockableToolsTest::projectExplorerUsesRtlFilteringAndSafeRoot()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString root = temporaryDirectory.filePath(QStringLiteral("project"));
    QVERIFY(QDir().mkpath(root));
    QFile matchingFile(QDir(root).filePath(QStringLiteral("matching.alif")));
    QVERIFY(matchingFile.open(QIODevice::WriteOnly));
    matchingFile.close();

    ProjectExplorerWidget explorer;
    explorer.resize(300, 520);
    explorer.setProjectRoot(root);
    explorer.show();
    QCOMPARE(explorer.layoutDirection(), Qt::RightToLeft);
    QCOMPARE(explorer.proxyModel()->projectRoot(), ProjectFileProxyModel::normalizedPath(root));
    QVERIFY(explorer.proxyModel()->isPathInsideProject(matchingFile.fileName()));
    QVERIFY(!explorer.proxyModel()->isPathInsideProject(QDir::tempPath()));

    auto* const filter = explorer.findChild<QLineEdit*>(QStringLiteral("ProjectExplorerFilterEdit"));
    QVERIFY(filter != nullptr);
    filter->setText(QStringLiteral("matching"));
    QTRY_COMPARE_WITH_TIMEOUT(explorer.proxyModel()->filterText(), QStringLiteral("matching"), 500);
    QVERIFY(explorer.treeView()->isVisible());
    QCOMPARE(explorer.treeView()->layoutDirection(), Qt::RightToLeft);
}

void DockableToolsTest::gitStatusServiceReportsReadOnlyFileStates()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString root = temporaryDirectory.filePath(QStringLiteral("repository"));
    QVERIFY(QDir().mkpath(root));

    const auto runGit = [&root](const QStringList& arguments) {
        QProcess process;
        process.setWorkingDirectory(root);
        process.start(QStringLiteral("git"), arguments);
        return process.waitForFinished(3000) && process.exitStatus() == QProcess::NormalExit
            && process.exitCode() == 0;
    };
    QVERIFY(runGit({QStringLiteral("init")}));

    QFile added(QDir(root).filePath(QStringLiteral("added.alif")));
    QVERIFY(added.open(QIODevice::WriteOnly));
    added.write("اطبع(1)\n");
    added.close();
    QVERIFY(runGit({QStringLiteral("add"), QStringLiteral("added.alif")}));

    QFile untracked(QDir(root).filePath(QStringLiteral("untracked.alif")));
    QVERIFY(untracked.open(QIODevice::WriteOnly));
    untracked.write("اطبع(2)\n");
    untracked.close();

    GitStatusService service;
    service.setProjectRoot(root);
    QTRY_VERIFY_WITH_TIMEOUT(service.isRepository(), 3000);
    QCOMPARE(service.statusForRelativePath(QStringLiteral("added.alif")), VersionControlState::Added);
    QCOMPARE(service.statusForRelativePath(QStringLiteral("untracked.alif")), VersionControlState::Untracked);
}

void DockableToolsTest::mainWindowBindsProjectExplorerToFolderRoot()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString root = temporaryDirectory.filePath(QStringLiteral("project"));
    QVERIFY(QDir().mkpath(root));

    Taif window({}, nullptr, true);
    window.resize(960, 680);
    window.show();
    window.loadFolder(root);

    auto* const explorer = window.findChild<ProjectExplorerWidget*>();
    QVERIFY(explorer != nullptr);
    QTRY_COMPARE_WITH_TIMEOUT(explorer->projectRoot(), ProjectFileProxyModel::normalizedPath(root), 1000);
    QVERIFY(explorer->isVisible());
    QVERIFY(explorer->treeView()->isVisible());
}

void DockableToolsTest::gitRepositoryServiceRefreshesAndStagesWorktreeFile()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString root = temporaryDirectory.filePath(QStringLiteral("repository"));
    QVERIFY(QDir().mkpath(root));
    const auto runGit = [&root](const QStringList& arguments) {
        QProcess process; process.setWorkingDirectory(root); process.start(QStringLiteral("git"), arguments);
        return process.waitForFinished(3000) && process.exitCode() == 0;
    };
    QVERIFY(runGit({QStringLiteral("init")}));
    QFile file(QDir(root).filePath(QStringLiteral("new.alif")));
    QVERIFY(file.open(QIODevice::WriteOnly)); file.write("اطبع(1)\n"); file.close();

    GitRepositoryService service;
    QSignalSpy operationSpy(&service, &GitRepositoryService::operationFinished);
    service.setProjectRoot(root);
    QTRY_VERIFY_WITH_TIMEOUT(service.snapshot().repository, 3000);
    QCOMPARE(service.statusForRelativePath(QStringLiteral("new.alif")), VersionControlState::Untracked);
    service.stage({QStringLiteral("new.alif")});
    QTRY_VERIFY_WITH_TIMEOUT(operationSpy.count() == 1, 3000);
    QVERIFY(operationSpy.takeFirst().at(0).value<GitCommandResult>().succeeded);
    QTRY_COMPARE_WITH_TIMEOUT(service.statusForRelativePath(QStringLiteral("new.alif")), VersionControlState::Added, 3000);
}

void DockableToolsTest::projectExplorerManualRefreshUpdatesGitStatus()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString root = temporaryDirectory.filePath(QStringLiteral("repository"));
    QVERIFY(QDir().mkpath(root));
    QProcess init; init.setWorkingDirectory(root); init.start(QStringLiteral("git"), {QStringLiteral("init")});
    QVERIFY(init.waitForFinished(3000)); QCOMPARE(init.exitCode(), 0);

    ProjectExplorerWidget explorer; explorer.setProjectRoot(root);
    QTRY_VERIFY_WITH_TIMEOUT(explorer.gitRepositoryService()->snapshot().repository, 3000);
    QFile file(QDir(root).filePath(QStringLiteral("refresh.alif")));
    QVERIFY(file.open(QIODevice::WriteOnly)); file.write("اطبع(3)\n"); file.close();
    explorer.refresh();
    QTRY_COMPARE_WITH_TIMEOUT(explorer.gitRepositoryService()->statusForRelativePath(QStringLiteral("refresh.alif")),
                              VersionControlState::Untracked, 3000);
}

void DockableToolsTest::mainWindowExposesRightSideGitPanel()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString root = temporaryDirectory.filePath(QStringLiteral("project"));
    QVERIFY(QDir().mkpath(root));
    Taif window({}, nullptr, true); window.resize(960, 680); window.show(); window.loadFolder(root);
    auto* const dock = window.findChild<QDockWidget*>(QStringLiteral("GitDock"));
    auto* const panel = window.findChild<GitPanelWidget*>();
    QVERIFY(dock != nullptr); QVERIFY(panel != nullptr); QVERIFY(!dock->isVisible());
    QVERIFY(QMetaObject::invokeMethod(&window, "showGitPanel", Qt::DirectConnection));
    QTRY_VERIFY_WITH_TIMEOUT(dock->isVisible(), 1000);
    QCOMPARE(panel->projectRoot(), ProjectFileProxyModel::normalizedPath(root));
}

void DockableToolsTest::gitPanelUsesDarkNavySurfaces()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString root = temporaryDirectory.filePath(QStringLiteral("project"));
    QVERIFY(QDir().mkpath(root));
    Taif window({}, nullptr, true); window.resize(1024, 720); window.show(); window.loadFolder(root);
    QVERIFY(QMetaObject::invokeMethod(&window, "showGitPanel", Qt::DirectConnection));
    auto* const panel = window.findChild<GitPanelWidget*>();
    QVERIFY(panel != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(panel->isVisible(), 1000);
    const QImage image = panel->grab().toImage();
    QVERIFY(!image.isNull());
    const QColor background = image.pixelColor(4, 4);
    QVERIFY2(background.red() < 50 && background.green() < 60 && background.blue() < 80,
             qPrintable(QStringLiteral("Unexpected Git panel background: %1,%2,%3")
                 .arg(background.red()).arg(background.green()).arg(background.blue())));
}

void DockableToolsTest::workspaceAutoCommandPolicyFailsClosed()
{
    QString reason;
    QVERIFY(!AiWorkspacePolicy::commandRequiresApproval(QStringLiteral("qmake"), &reason));
    QVERIFY(!AiWorkspacePolicy::commandRequiresApproval(QStringLiteral("nmake"), &reason));
    QVERIFY(!AiWorkspacePolicy::commandRequiresApproval(QStringLiteral("cmake --build build"), &reason));
    QVERIFY(!AiWorkspacePolicy::commandRequiresApproval(QStringLiteral("ctest --test-dir build"), &reason));
    QVERIFY(!AiWorkspacePolicy::commandRequiresApproval(QStringLiteral("git diff --stat"), &reason));

    const QStringList blocked = {
        QStringLiteral("python -c print(1)"), QStringLiteral("npm test"),
        QStringLiteral("git push"), QStringLiteral("nmake clean"),
        QStringLiteral("cmake --build build --target install"), QStringLiteral("qmake project.pro"),
        QStringLiteral("cmd.exe /C nmake"), QStringLiteral("nmake && git status"),
        QStringLiteral("ctest $HOME") };
    for (const QString& command : blocked) {
        QVERIFY2(AiWorkspacePolicy::commandRequiresApproval(command, &reason),
                 qPrintable(QStringLiteral("expected approval for: %1").arg(command)));
        QVERIFY(!reason.isEmpty());
    }
}

void DockableToolsTest::aiLineDiffAlignsArabicAndChangedRowsDeterministically()
{
    const AiLineDiffResult result = AiLineDiff::compare(
        QStringLiteral("أولاً\nثانياً\nثالثاً\n"),
        QStringLiteral("أولاً\nثانياً المعدلة\nثالثاً\nرابعاً\n"));
    QCOMPARE(result.summary.unchangedLines, 3);
    QCOMPARE(result.summary.removedLines, 1);
    QCOMPARE(result.summary.addedLines, 2);
    QVERIFY(!result.summary.usedWholeDocumentFallback);
    QCOMPARE(result.rows.size(), 6);
    QCOMPARE(result.rows.at(0).kind, AiLineDiffRow::Kind::Unchanged);
    QCOMPARE(result.rows.at(1).kind, AiLineDiffRow::Kind::Removed);
    QCOMPARE(result.rows.at(2).kind, AiLineDiffRow::Kind::Added);
    QCOMPARE(result.rows.last().kind, AiLineDiffRow::Kind::Unchanged);

    const AiLineDiffResult unchanged = AiLineDiff::compare(QStringLiteral("سطر\n"), QStringLiteral("سطر\n"));
    QCOMPARE(unchanged.summary.unchangedLines, 2);
    QCOMPARE(unchanged.summary.addedLines, 0);
    QCOMPARE(unchanged.summary.removedLines, 0);
}

void DockableToolsTest::aiTextPatchAppliesAnchoredLineEditsAndRejectsWholeFileOrStaleRanges()
{
    const QString source = QStringLiteral("سطر أول\nسطر ثان\nسطر ثالث\nسطر رابع\n");
    const QJsonArray edits{QJsonObject{{QStringLiteral("start_line"), 2},
                                       {QStringLiteral("end_line"), 3},
                                       {QStringLiteral("expected_text"), QStringLiteral("سطر ثان\nسطر ثالث")},
                                       {QStringLiteral("replacement"), QStringLiteral("سطر معدّل")}}};
    const AiTextPatchResult result = AiTextPatch::applyAnchoredLineEdits(source, edits);
    QVERIFY(result.succeeded);
    QCOMPARE(result.text, QStringLiteral("سطر أول\nسطر معدّل\nسطر رابع\n"));

    QJsonArray staleEdits = edits;
    QJsonObject staleEdit = staleEdits.first().toObject();
    staleEdit.insert(QStringLiteral("expected_text"), QStringLiteral("نص مختلف"));
    staleEdits[0] = staleEdit;
    QVERIFY(!AiTextPatch::applyAnchoredLineEdits(source, staleEdits).succeeded);

    const QJsonArray wholeFile{QJsonObject{{QStringLiteral("start_line"), 1},
                                           {QStringLiteral("end_line"), 4},
                                           {QStringLiteral("expected_text"), QStringLiteral("سطر أول\nسطر ثان\nسطر ثالث\nسطر رابع")},
                                           {QStringLiteral("replacement"), QStringLiteral("استبدال كامل")}}};
    QVERIFY(!AiTextPatch::applyAnchoredLineEdits(source, wholeFile).succeeded);
}

void DockableToolsTest::aiTextPatchPreservesCrLfAndFinalNewline()
{
    const QString source = QStringLiteral("قبل\r\nاطبع(1)\r\nبعد\r\n");
    const QJsonArray edits{QJsonObject{{QStringLiteral("start_line"), 2},
                                       {QStringLiteral("end_line"), 2},
                                       {QStringLiteral("expected_text"), QStringLiteral("اطبع(1)")},
                                       {QStringLiteral("replacement"), QStringLiteral("اطبع(2)")}}};
    const AiTextPatchResult result = AiTextPatch::applyAnchoredLineEdits(source, edits);
    QVERIFY(result.succeeded);
    QCOMPARE(result.text, QStringLiteral("قبل\r\nاطبع(2)\r\nبعد\r\n"));

    const AiTextPatchResult noFinalNewline = AiTextPatch::applyAnchoredLineEdits(
        QStringLiteral("أول\nثان"),
        QJsonArray{QJsonObject{{QStringLiteral("start_line"), 2},
                                {QStringLiteral("end_line"), 2},
                                {QStringLiteral("expected_text"), QStringLiteral("ثان")},
                                {QStringLiteral("replacement"), QStringLiteral("آخر")}}});
    QVERIFY(noFinalNewline.succeeded);
    QCOMPARE(noFinalNewline.text, QStringLiteral("أول\nآخر"));
}

void DockableToolsTest::aiAssistantSettingsUseWorkspaceAutoAndBoundLongTimeouts()
{
    const AiAssistantSettings defaults = AiAssistantSettingsStore::defaults();
    QCOMPARE(defaults.requestTimeoutMilliseconds, 600000);
    QCOMPARE(defaults.commandTimeoutMilliseconds, 300000);
    QCOMPARE(defaults.autonomyMode, AiAutonomyMode::WorkspaceAuto);

    AiAssistantSettings invalid = defaults;
    invalid.requestTimeoutMilliseconds = 1;
    invalid.commandTimeoutMilliseconds = 99999999;
    invalid.autonomyMode = static_cast<AiAutonomyMode>(42);
    const AiAssistantSettings normalized = AiAssistantSettingsStore::normalize(invalid);
    QCOMPARE(normalized.requestTimeoutMilliseconds, 30000);
    QCOMPARE(normalized.commandTimeoutMilliseconds, 1800000);
    QCOMPARE(normalized.autonomyMode, AiAutonomyMode::WorkspaceAuto);
}

void DockableToolsTest::lmStudioClientSerializesAssistantToolCallsForContinuation()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QByteArray request;
    QPointer<QTcpSocket> peer;
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
        peer = server.nextPendingConnection();
        connect(peer, &QTcpSocket::readyRead, &server, [&]() {
            request.append(peer->readAll());
            const int separator = request.indexOf("\r\n\r\n");
            if (separator < 0) {
                return;
            }
            const QByteArray headers = request.left(separator);
            const QRegularExpression contentLengthExpression(QStringLiteral("Content-Length:\\s*(\\d+)"),
                                                              QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch match = contentLengthExpression.match(QString::fromLatin1(headers));
            QVERIFY(match.hasMatch());
            const int contentLength = match.captured(1).toInt();
            if (request.size() < separator + 4 + contentLength) {
                return;
            }
            peer->write("HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nConnection: close\r\n\r\ndata: [DONE]\n\n");
            peer->disconnectFromHost();
        });
    });

    AiAssistantSettings settings;
    settings.endpointUrl = QStringLiteral("http://127.0.0.1:%1/v1").arg(server.serverPort());
    settings.requestTimeoutMilliseconds = 1000;
    LmStudioClient client;
    client.setSettings(settings);
    QSignalSpy finished(&client, &LmStudioClient::streamFinished);

    AiChatMessage assistant;
    assistant.role = AiChatRole::Assistant;
    assistant.toolCalls.append(QJsonObject{
        {QStringLiteral("id"), QStringLiteral("call-1")},
        {QStringLiteral("type"), QStringLiteral("function")},
        {QStringLiteral("function"), QJsonObject{{QStringLiteral("name"), QStringLiteral("read_project_file")},
                                                    {QStringLiteral("arguments"), QStringLiteral("{\\\"path\\\":\\\"main.alif\\\"}")}}}});
    AiChatMessage result;
    result.role = AiChatRole::Tool;
    result.toolCallId = QStringLiteral("call-1");
    result.name = QStringLiteral("read_project_file");
    result.content = QStringLiteral("read completed");
    client.streamChat({assistant, result}, QStringLiteral("local-model"));

    QTRY_VERIFY_WITH_TIMEOUT(request.contains("\r\n\r\n"), 1500);
    const int separator = request.indexOf("\r\n\r\n");
    const QJsonDocument document = QJsonDocument::fromJson(request.mid(separator + 4));
    QVERIFY(document.isObject());
    const QJsonArray messages = document.object().value(QStringLiteral("messages")).toArray();
    QCOMPARE(messages.size(), 2);
    const QJsonObject serializedAssistant = messages.at(0).toObject();
    QCOMPARE(serializedAssistant.value(QStringLiteral("role")).toString(), QStringLiteral("assistant"));
    const QJsonArray toolCalls = serializedAssistant.value(QStringLiteral("tool_calls")).toArray();
    QCOMPARE(toolCalls.size(), 1);
    QCOMPARE(toolCalls.first().toObject().value(QStringLiteral("id")).toString(), QStringLiteral("call-1"));
    QCOMPARE(messages.at(1).toObject().value(QStringLiteral("tool_call_id")).toString(), QStringLiteral("call-1"));
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 1500);
}

void DockableToolsTest::workspaceAutoCompletesSafeReadAndContinues()
{
    QTemporaryDir project;
    QVERIFY(project.isValid());
    QFile source(QDir(project.path()).filePath(QStringLiteral("main.alif")));
    QVERIFY(source.open(QIODevice::WriteOnly | QIODevice::Text));
    source.write("قبل\nاطبع(1)\nبعد\n");
    source.close();

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QHash<QTcpSocket*, QByteArray> buffers;
    int responseCount = 0;
    const auto sendSse = [](QTcpSocket* const socket, const QJsonObject& delta) {
        const QJsonObject choice{{QStringLiteral("delta"), delta}};
        const QJsonObject envelope{{QStringLiteral("choices"), QJsonArray{choice}}};
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nConnection: close\r\n\r\ndata: ");
        socket->write(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
        socket->write("\n\ndata: [DONE]\n\n");
        socket->disconnectFromHost();
    };
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
        auto* const socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, &server, [&, socket]() {
            QByteArray& buffer = buffers[socket];
            buffer.append(socket->readAll());
            const int separator = buffer.indexOf("\r\n\r\n");
            if (separator < 0) {
                return;
            }
            const QRegularExpression expression(QStringLiteral("Content-Length:\\s*(\\d+)"),
                                                QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch match = expression.match(QString::fromLatin1(buffer.left(separator)));
            QVERIFY(match.hasMatch());
            if (buffer.size() < separator + 4 + match.captured(1).toInt()) {
                return;
            }
            buffers.remove(socket);
            if (responseCount++ == 0) {
                const QString arguments = QString::fromUtf8(QJsonDocument(
                    QJsonObject{{QStringLiteral("path"), QStringLiteral("main.alif")}}).toJson(QJsonDocument::Compact));
                const QJsonObject function{{QStringLiteral("name"), QStringLiteral("read_project_file")},
                                           {QStringLiteral("arguments"), arguments}};
                const QJsonObject toolCall{{QStringLiteral("index"), 0}, {QStringLiteral("id"), QStringLiteral("read-1")},
                                           {QStringLiteral("type"), QStringLiteral("function")}, {QStringLiteral("function"), function}};
                sendSse(socket, QJsonObject{{QStringLiteral("tool_calls"), QJsonArray{toolCall}}});
            } else {
                sendSse(socket, QJsonObject{{QStringLiteral("content"), QStringLiteral("تمت مراجعة الملف بأمان.")}});
            }
        });
    });

    AiAgentController controller;
    AiAssistantSettings settings = controller.client()->settings();
    settings.endpointUrl = QStringLiteral("http://127.0.0.1:%1/v1").arg(server.serverPort());
    settings.requestTimeoutMilliseconds = 2000;
    settings.autonomyMode = AiAutonomyMode::WorkspaceAuto;
    controller.client()->setSettings(settings);
    controller.setProjectRoot(project.path());
    controller.setSelectedModel(QStringLiteral("local-model"));
    QSignalSpy activity(&controller, &AiAgentController::activityAdded);
    controller.submitPrompt(QStringLiteral("اقرأ الملف ثم لخّصه."), false, false);

    QTRY_COMPARE_WITH_TIMEOUT(controller.state(), AiAgentState::Idle, 4000);
    const QVector<AiChatMessage> messages = controller.messages();
    QVERIFY(messages.size() >= 5);
    bool sawAssistantToolBatch = false;
    bool sawToolResult = false;
    bool sawFinalResponse = false;
    for (const AiChatMessage& message : messages) {
        sawAssistantToolBatch = sawAssistantToolBatch || !message.toolCalls.isEmpty();
        sawToolResult = sawToolResult || (message.role == AiChatRole::Tool && message.toolCallId == QStringLiteral("read-1"));
        sawFinalResponse = sawFinalResponse || message.content.contains(QStringLiteral("تمت مراجعة الملف"));
    }
    QVERIFY(sawAssistantToolBatch);
    QVERIFY(sawToolResult);
    QVERIFY(sawFinalResponse);
    QVERIFY(responseCount >= 2);
    bool sawAutomaticActivity = false;
    for (const QList<QVariant>& signalArguments : activity) {
        sawAutomaticActivity = sawAutomaticActivity
            || signalArguments.constFirst().value<AiActivityEntry>().kind == AiActivityKind::AutoExecuted;
    }
    QVERIFY(sawAutomaticActivity);
}

void DockableToolsTest::workspaceAutoProtectsUnsavedOpenFile()
{
    QTemporaryDir project;
    QVERIFY(project.isValid());
    const QString sourcePath = QDir(project.path()).filePath(QStringLiteral("main.alif"));
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly | QIODevice::Text));
    source.write("قبل\nاطبع(1)\nبعد\n");
    source.close();

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QByteArray request;
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
        auto* const socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, &server, [&, socket]() {
            request.append(socket->readAll());
            const int separator = request.indexOf("\r\n\r\n");
            if (separator < 0) {
                return;
            }
            const QRegularExpression expression(QStringLiteral("Content-Length:\\s*(\\d+)"),
                                                QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch match = expression.match(QString::fromLatin1(request.left(separator)));
            QVERIFY(match.hasMatch());
            if (request.size() < separator + 4 + match.captured(1).toInt()) {
                return;
            }
            const QString arguments = QString::fromUtf8(QJsonDocument(QJsonObject{
                {QStringLiteral("path"), QStringLiteral("main.alif")},
                {QStringLiteral("edits"), QJsonArray{QJsonObject{{QStringLiteral("start_line"), 2},
                                                                    {QStringLiteral("end_line"), 2},
                                                                    {QStringLiteral("expected_text"), QStringLiteral("اطبع(1)")},
                                                                    {QStringLiteral("replacement"), QStringLiteral("اطبع(2)")}}}}}).toJson(QJsonDocument::Compact));
            const QJsonObject function{{QStringLiteral("name"), QStringLiteral("propose_file_patch")},
                                       {QStringLiteral("arguments"), arguments}};
            const QJsonObject toolCall{{QStringLiteral("index"), 0}, {QStringLiteral("id"), QStringLiteral("patch-1")},
                                       {QStringLiteral("type"), QStringLiteral("function")}, {QStringLiteral("function"), function}};
            const QJsonObject choice{{QStringLiteral("delta"), QJsonObject{{QStringLiteral("tool_calls"), QJsonArray{toolCall}}}}};
            const QJsonObject envelope{{QStringLiteral("choices"), QJsonArray{choice}}};
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nConnection: close\r\n\r\ndata: ");
            socket->write(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
            socket->write("\n\ndata: [DONE]\n\n");
            socket->disconnectFromHost();
        });
    });

    AiAgentController controller;
    AiAssistantSettings settings = controller.client()->settings();
    settings.endpointUrl = QStringLiteral("http://127.0.0.1:%1/v1").arg(server.serverPort());
    settings.requestTimeoutMilliseconds = 2000;
    settings.autonomyMode = AiAutonomyMode::WorkspaceAuto;
    controller.client()->setSettings(settings);
    controller.setProjectRoot(project.path());
    controller.setModifiedOpenFiles({sourcePath});
    controller.setSelectedModel(QStringLiteral("local-model"));
    QSignalSpy reviews(&controller, &AiAgentController::patchReviewRequested);
    QSignalSpy toolResults(&controller, &AiAgentController::toolResultReady);
    controller.submitPrompt(QStringLiteral("حدّث الملف."), false, false);

    QTRY_COMPARE_WITH_TIMEOUT(reviews.count(), 1, 2000);
    QCOMPARE(controller.state(), AiAgentState::AwaitingApproval);
    const AiPatchReviewRequest review = reviews.constFirst().constFirst().value<AiPatchReviewRequest>();
    QCOMPARE(review.toolCall.id, QStringLiteral("patch-1"));
    controller.acceptPatchReview(review.reviewId);
    QTRY_COMPARE_WITH_TIMEOUT(toolResults.count(), 1, 2000);
    QVERIFY(!toolResults.constFirst().at(2).toBool());
    controller.stop();
    QFile unchanged(sourcePath);
    QVERIFY(unchanged.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(unchanged.readAll()), QStringLiteral("قبل\nاطبع(1)\nبعد\n"));
}

void DockableToolsTest::aiPatchStagesForReviewAndWritesOnlyAfterAcceptance()
{
    QTemporaryDir project;
    QVERIFY(project.isValid());
    const QString sourcePath = QDir(project.path()).filePath(QStringLiteral("main.alif"));
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly | QIODevice::Text));
    source.write("قبل\nاطبع(1)\nبعد\n");
    source.close();

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QByteArray request;
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
        auto* const socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, &server, [&, socket]() {
            request.append(socket->readAll());
            const int separator = request.indexOf("\r\n\r\n");
            if (separator < 0) return;
            const QRegularExpression expression(QStringLiteral("Content-Length:\\s*(\\d+)"),
                                                QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch match = expression.match(QString::fromLatin1(request.left(separator)));
            QVERIFY(match.hasMatch());
            if (request.size() < separator + 4 + match.captured(1).toInt()) return;
            const QString arguments = QString::fromUtf8(QJsonDocument(QJsonObject{
                {QStringLiteral("path"), QStringLiteral("main.alif")},
                {QStringLiteral("edits"), QJsonArray{QJsonObject{{QStringLiteral("start_line"), 2},
                                                                    {QStringLiteral("end_line"), 2},
                                                                    {QStringLiteral("expected_text"), QStringLiteral("اطبع(1)")},
                                                                    {QStringLiteral("replacement"), QStringLiteral("اطبع(2)")}}}}}).toJson(QJsonDocument::Compact));
            const QJsonObject function{{QStringLiteral("name"), QStringLiteral("propose_file_patch")},
                                       {QStringLiteral("arguments"), arguments}};
            const QJsonObject toolCall{{QStringLiteral("index"), 0}, {QStringLiteral("id"), QStringLiteral("review-patch-1")},
                                       {QStringLiteral("type"), QStringLiteral("function")}, {QStringLiteral("function"), function}};
            const QJsonObject choice{{QStringLiteral("delta"), QJsonObject{{QStringLiteral("tool_calls"), QJsonArray{toolCall}}}}};
            const QJsonObject envelope{{QStringLiteral("choices"), QJsonArray{choice}}};
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nConnection: close\r\n\r\ndata: ");
            socket->write(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
            socket->write("\n\ndata: [DONE]\n\n");
            socket->disconnectFromHost();
        });
    });

    AiAgentController controller;
    AiAssistantSettings settings = controller.client()->settings();
    settings.endpointUrl = QStringLiteral("http://127.0.0.1:%1/v1").arg(server.serverPort());
    settings.requestTimeoutMilliseconds = 2000;
    settings.autonomyMode = AiAutonomyMode::Manual;
    controller.client()->setSettings(settings);
    controller.setProjectRoot(project.path());
    controller.setSelectedModel(QStringLiteral("local-model"));
    QSignalSpy reviews(&controller, &AiAgentController::patchReviewRequested);
    controller.submitPrompt(QStringLiteral("حدّث الملف."), false, false);

    QTRY_COMPARE_WITH_TIMEOUT(reviews.count(), 1, 2000);
    const AiPatchReviewRequest review = reviews.constFirst().constFirst().value<AiPatchReviewRequest>();
    QCOMPARE(review.relativePath, QStringLiteral("main.alif"));
    QCOMPARE(review.originalText, QStringLiteral("قبل\r\nاطبع(1)\r\nبعد\r\n"));
    QCOMPARE(review.proposedText, QStringLiteral("قبل\r\nاطبع(2)\r\nبعد\r\n"));
    QVERIFY(review.isValid());
    QCOMPARE(controller.state(), AiAgentState::AwaitingApproval);
    QFile unchanged(sourcePath);
    QVERIFY(unchanged.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(unchanged.readAll()), QStringLiteral("قبل\nاطبع(1)\nبعد\n"));
    unchanged.close();

    controller.acceptPatchReview(review.reviewId);
    QTRY_VERIFY(controller.state() == AiAgentState::Idle);
    QFile updated(sourcePath);
    QVERIFY(updated.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(updated.readAll()), QStringLiteral("قبل\nاطبع(2)\nبعد\n"));
}

void DockableToolsTest::aiPatchReviewWidgetPresentsReadOnlyLtrSplitAndAcceptAction()
{
    AiPatchReviewWidget widget;
    widget.resize(1000, 640);
    widget.show();
    AiPatchReviewRequest review;
    review.reviewId = QStringLiteral("review-1");
    review.toolCall.id = QStringLiteral("call-1");
    review.toolCall.kind = AiToolKind::ProposeFilePatch;
    review.toolCall.name = QStringLiteral("propose_file_patch");
    review.relativePath = QStringLiteral("source/main.alif");
    review.absolutePath = QStringLiteral("C:/workspace/source/main.alif");
    review.originalSha256 = QStringLiteral("snapshot");
    review.originalText = QStringLiteral("اطبع(1)\n");
    review.proposedText = QStringLiteral("اطبع(2)\n");
    widget.setReview(review);

    QCOMPARE(widget.reviewId(), review.reviewId);
    QVERIFY(widget.originalPane()->isReadOnly());
    QVERIFY(widget.proposedPane()->isReadOnly());
    QCOMPARE(widget.originalPane()->layoutDirection(), Qt::LeftToRight);
    QCOMPARE(widget.proposedPane()->layoutDirection(), Qt::LeftToRight);
    QCOMPARE(widget.originalPane()->toPlainText(), review.originalText);
    QCOMPARE(widget.proposedPane()->toPlainText(), review.proposedText);
    auto* const accept = widget.findChild<QPushButton*>(QStringLiteral("AiPatchReviewAccept"));
    auto* const reject = widget.findChild<QPushButton*>(QStringLiteral("AiPatchReviewReject"));
    QVERIFY(accept != nullptr);
    QVERIFY(reject != nullptr);
    QVERIFY(accept->isEnabled());
    QVERIFY(reject->isEnabled());
    QSignalSpy accepted(&widget, &AiPatchReviewWidget::acceptRequested);
    accept->click();
    QCOMPARE(accepted.count(), 1);
    QCOMPARE(accepted.constFirst().constFirst().toString(), review.reviewId);
}

void DockableToolsTest::aiPatchPreviewAppearsAndUpdatesFromStreamedArguments()
{
    QTemporaryDir project;
    QVERIFY(project.isValid());
    QFile source(QDir(project.path()).filePath(QStringLiteral("main.alif")));
    QVERIFY(source.open(QIODevice::WriteOnly | QIODevice::Text));
    source.write("قبل\nاطبع(1)\nبعد\n");
    source.close();

    AiAgentController controller;
    controller.setProjectRoot(project.path());
    QSignalSpy previews(&controller, &AiAgentController::patchReviewPreviewUpdated);
    QVERIFY(QMetaObject::invokeMethod(controller.client(), "toolCallDelta", Qt::DirectConnection,
                                      Q_ARG(QString, QStringLiteral("live-patch-1")),
                                      Q_ARG(QString, QStringLiteral("propose_file_patch")),
                                      Q_ARG(QString, QStringLiteral("{\"path\":\"main.alif\",\"edits\":[{\"start_line\":2,\"end_line\":2,\"expected_text\":\"اطبع(1)\",\"replacement\":\"اطبع("))));
    QTRY_COMPARE(previews.count(), 1);
    const AiPatchReviewRequest firstPreview = previews.constFirst().constFirst().value<AiPatchReviewRequest>();
    QVERIFY(firstPreview.isStreamingPreview);
    QCOMPARE(firstPreview.relativePath, QStringLiteral("main.alif"));
    QCOMPARE(firstPreview.originalText, QStringLiteral("قبل\r\nاطبع(1)\r\nبعد\r\n"));
    QCOMPARE(firstPreview.proposedText, firstPreview.originalText);
    QCOMPARE(firstPreview.streamingStartLine, 2);
    QCOMPARE(firstPreview.streamingEndLine, 2);
    QCOMPARE(firstPreview.streamingExpectedText, QStringLiteral("اطبع(1)"));
    QCOMPARE(firstPreview.streamingReplacement, QStringLiteral("اطبع("));
    QVERIFY(!firstPreview.isValid());

    QVERIFY(QMetaObject::invokeMethod(controller.client(), "toolCallDelta", Qt::DirectConnection,
                                      Q_ARG(QString, QStringLiteral("live-patch-1")),
                                      Q_ARG(QString, QString()),
                                      Q_ARG(QString, QStringLiteral("2)\"}]}"))));
    QTRY_COMPARE(previews.count(), 2);
    const AiPatchReviewRequest finalPreview = previews.constLast().constFirst().value<AiPatchReviewRequest>();
    QVERIFY(finalPreview.isStreamingPreview);
    QCOMPARE(finalPreview.proposedText, finalPreview.originalText);
    QCOMPARE(finalPreview.streamingReplacement, QStringLiteral("اطبع(2)"));
    QFile unchanged(source.fileName());
    QVERIFY(unchanged.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(unchanged.readAll()), QStringLiteral("قبل\nاطبع(1)\nبعد\n"));
}

void DockableToolsTest::aiPatchReviewWidgetStreamsProposedTokensBeforeEnablingAcceptance()
{
    AiPatchReviewWidget widget;
    widget.resize(1000, 640);
    widget.show();
    AiPatchReviewRequest preview;
    preview.reviewId = QStringLiteral("stream-review");
    preview.toolCall.id = QStringLiteral("stream-call");
    preview.toolCall.kind = AiToolKind::ProposeFilePatch;
    preview.toolCall.name = QStringLiteral("propose_file_patch");
    preview.relativePath = QStringLiteral("main.alif");
    preview.absolutePath = QStringLiteral("C:/workspace/main.alif");
    preview.originalSha256 = QStringLiteral("snapshot");
    preview.originalText = QStringLiteral("قبل\nاطبع(1)\nبعد\n");
    preview.proposedText = preview.originalText;
    preview.isStreamingPreview = true;
    preview.streamingStartLine = 2;
    preview.streamingEndLine = 2;
    preview.streamingExpectedText = QStringLiteral("اطبع(1)");
    preview.streamingReplacement = QStringLiteral("اطبع(2)");
    widget.setReview(preview);

    auto* const accept = widget.findChild<QPushButton*>(QStringLiteral("AiPatchReviewAccept"));
    QVERIFY(accept != nullptr);
    QVERIFY(!accept->isEnabled());
    QCOMPARE(widget.originalPane()->toPlainText(), preview.originalText);
    QCOMPARE(widget.proposedPane()->toPlainText(), QStringLiteral("قبل\nاطبع(2)\nبعد\n"));
    QVERIFY(!widget.proposedPane()->toPlainText().contains(QStringLiteral("اطبع(1)")));

    AiPatchReviewRequest finalReview = preview;
    finalReview.reviewId = QStringLiteral("final-review");
    finalReview.isStreamingPreview = false;
    finalReview.proposedText = QStringLiteral("قبل\nاطبع(2)\nبعد\n");
    widget.setReview(finalReview);
    QTRY_COMPARE_WITH_TIMEOUT(widget.proposedPane()->toPlainText(), finalReview.proposedText, 1000);
    QCOMPARE(widget.reviewId(), finalReview.reviewId);
    QVERIFY(accept->isEnabled());
}

void DockableToolsTest::mainWindowSwitchesToAiPatchReviewWorkspace()
{
    Taif window({}, nullptr, true);
    window.resize(1100, 760);
    window.show();
    auto* const stack = window.findChild<QStackedWidget*>(QStringLiteral("EditorWorkspaceStack"));
    auto* const reviewWidget = window.findChild<AiPatchReviewWidget*>();
    auto* const aiPanel = window.findChild<AiChatPanel*>();
    QVERIFY(stack != nullptr);
    QVERIFY(reviewWidget != nullptr);
    QVERIFY(aiPanel != nullptr);
    QVERIFY(stack->currentWidget() != reviewWidget);

    AiPatchReviewRequest review;
    review.reviewId = QStringLiteral("window-review");
    review.toolCall.id = QStringLiteral("window-call");
    review.toolCall.kind = AiToolKind::ProposeFilePatch;
    review.toolCall.name = QStringLiteral("propose_file_patch");
    review.relativePath = QStringLiteral("main.alif");
    review.absolutePath = QStringLiteral("C:/workspace/main.alif");
    review.originalSha256 = QStringLiteral("snapshot");
    review.originalText = QStringLiteral("اطبع(1)\n");
    review.proposedText = QStringLiteral("اطبع(2)\n");
    AiPatchReviewRequest preview = review;
    preview.reviewId = QStringLiteral("stream-window-review");
    preview.isStreamingPreview = true;
    QVERIFY(QMetaObject::invokeMethod(aiPanel->controller(), "patchReviewPreviewUpdated", Qt::DirectConnection,
                                      Q_ARG(AiPatchReviewRequest, preview)));
    QTRY_COMPARE(stack->currentWidget(), static_cast<QWidget*>(reviewWidget));
    QCOMPARE(reviewWidget->reviewId(), preview.reviewId);
    auto* const accept = reviewWidget->findChild<QPushButton*>(QStringLiteral("AiPatchReviewAccept"));
    QVERIFY(accept != nullptr);
    QVERIFY(!accept->isEnabled());

    QVERIFY(QMetaObject::invokeMethod(aiPanel->controller(), "patchReviewRequested", Qt::DirectConnection,
                                      Q_ARG(AiPatchReviewRequest, review)));
    QTRY_COMPARE(stack->currentWidget(), static_cast<QWidget*>(reviewWidget));
    QTRY_COMPARE_WITH_TIMEOUT(reviewWidget->reviewId(), review.reviewId, 1000);
    QVERIFY(accept->isEnabled());

    QVERIFY(QMetaObject::invokeMethod(aiPanel->controller(), "patchReviewResolved", Qt::DirectConnection,
                                      Q_ARG(QString, review.reviewId), Q_ARG(bool, false)));
    QTRY_VERIFY(stack->currentWidget() != reviewWidget);
}

void DockableToolsTest::aiAgentControllerConstructsAndDestroys()
{
    AiAgentController controller;
    QCOMPARE(controller.state(), AiAgentState::Idle);
    QVERIFY(controller.client() != nullptr);
    QVERIFY(!controller.client()->hasActiveRequest());
}

void DockableToolsTest::aiChatPanelConstructsAndDestroys()
{
    AiChatPanel panel;
    QCOMPARE(panel.layoutDirection(), Qt::RightToLeft);
    QVERIFY(panel.controller() != nullptr);
}

void DockableToolsTest::aiPanelPresentsWorkspaceAutoControlsAndHidesRawToolPayload()
{
    AiChatPanel panel;
    auto* const workspaceAuto = panel.findChild<QCheckBox*>(QStringLiteral("AiWorkspaceAuto"));
    auto* const timeout = panel.findChild<QComboBox*>(QStringLiteral("AiTimeoutCombo"));
    auto* const transcript = panel.findChild<QTextBrowser*>(QStringLiteral("AiChatTranscript"));
    QVERIFY(workspaceAuto != nullptr);
    QVERIFY(timeout != nullptr);
    QVERIFY(transcript != nullptr);
    QVERIFY(workspaceAuto->isChecked());
    QCOMPARE(timeout->currentData().toInt(), 600000);

    AiChatMessage toolMessage;
    toolMessage.role = AiChatRole::Tool;
    toolMessage.name = QStringLiteral("propose_file_patch");
    toolMessage.content = QStringLiteral("const QString internalSource = \\\"must not render in transcript\\\";");
    QVERIFY(QMetaObject::invokeMethod(panel.controller(), "messageAdded", Qt::DirectConnection,
                                      Q_ARG(AiChatMessage, toolMessage)));
    QTRY_VERIFY(transcript->toPlainText().contains(QStringLiteral("اكتملت خطوة")));
    QVERIFY(!transcript->toPlainText().contains(QStringLiteral("internalSource")));

    AiChatMessage assistantMessage;
    assistantMessage.role = AiChatRole::Assistant;
    assistantMessage.content = QStringLiteral("# عنوان الاستجابة\n\n**ملخص مهم**\n\n```cpp\nint value = 1;\n```\n\n<script>unsafe</script>");
    QVERIFY(QMetaObject::invokeMethod(panel.controller(), "messageAdded", Qt::DirectConnection,
                                      Q_ARG(AiChatMessage, assistantMessage)));
    QTRY_VERIFY(transcript->toPlainText().contains(QStringLiteral("عنوان الاستجابة")));
    QVERIFY(transcript->toHtml().contains(QStringLiteral("<h1")));
    QVERIFY(transcript->toHtml().contains(QStringLiteral("int value = 1;")));
    QVERIFY(!transcript->toPlainText().contains(QStringLiteral("**ملخص مهم**")));
    QVERIFY(!transcript->toHtml().contains(QStringLiteral("<script>unsafe</script>")));
    QCOMPARE(transcript->layoutDirection(), Qt::RightToLeft);
    QCOMPARE(transcript->document()->defaultTextOption().alignment() & Qt::AlignHorizontal_Mask,
             Qt::AlignRight);

    panel.resize(420, 480);
    panel.show();
    QString streamed;
    for (int index = 0; index < 80; ++index) {
        streamed += QStringLiteral("فقرة بث رقم %1 تعرض نصاً متصلاً للمراجعة.\n\n").arg(index);
    }
    QVERIFY(QMetaObject::invokeMethod(panel.controller(), "assistantTextUpdated", Qt::DirectConnection,
                                      Q_ARG(QString, streamed)));
    auto* const scrollBar = transcript->verticalScrollBar();
    QTRY_VERIFY(scrollBar->maximum() > 0);
    QTRY_COMPARE(scrollBar->value(), scrollBar->maximum());
    streamed += QStringLiteral("\n\nفقرة أخيرة أثناء البث.");
    QVERIFY(QMetaObject::invokeMethod(panel.controller(), "assistantTextUpdated", Qt::DirectConnection,
                                      Q_ARG(QString, streamed)));
    QTRY_COMPARE(scrollBar->value(), scrollBar->maximum());
}

void DockableToolsTest::aiChatPanelUsesLeftDockRtlSurface()
{
    Taif window({}, nullptr, true);
    window.resize(1100, 760);
    window.show();

    auto* const dock = window.findChild<QDockWidget*>(QStringLiteral("AiChatDock"));
    QVERIFY(dock != nullptr);
    QCOMPARE(window.dockWidgetArea(dock), Qt::LeftDockWidgetArea);
    QVERIFY(!dock->isVisible());
    auto* const panel = dock->findChild<AiChatPanel*>(QStringLiteral("AiChatPanel"));
    QVERIFY(panel != nullptr);
    QCOMPARE(panel->layoutDirection(), Qt::RightToLeft);
    QVERIFY(panel->findChild<QPlainTextEdit*>(QStringLiteral("AiChatComposer")) != nullptr);
    QVERIFY(panel->findChild<QTextBrowser*>(QStringLiteral("AiChatTranscript")) != nullptr);
    auto* const approvals = panel->findChild<QListWidget*>(QStringLiteral("AiApprovalList"));
    QVERIFY(approvals != nullptr);
    QVERIFY(approvals->minimumHeight() >= 132);

    auto* const applicationMenu = window.findChild<TMenuBar*>();
    QVERIFY(applicationMenu != nullptr);
    QVERIFY(applicationMenu->aiAssistantAction != nullptr);
    applicationMenu->aiAssistantAction->trigger();
    QTRY_VERIFY(dock->isVisible());
    QVERIFY(applicationMenu->aiAssistantAction->isChecked());
    const QImage image = panel->grab().toImage();
    QVERIFY(!image.isNull());
    const QColor background = image.pixelColor(4, 4);
    QVERIFY2(background.red() < 35 && background.green() < 45 && background.blue() < 65,
             qPrintable(QStringLiteral("Unexpected AI panel background: %1,%2,%3")
                 .arg(background.red()).arg(background.green()).arg(background.blue())));

    const AiAssistantSettings defaults = AiAssistantSettingsStore::defaults();
    QVERIFY(AiAssistantSettingsStore::isLoopbackEndpoint(defaults.endpointUrl));
}

void DockableToolsTest::multiCursorSelectsOccurrencesAndEditsInOneUndoStep()
{
    TEditor editor; editor.resize(640, 300); editor.show(); editor.setPlainText(QStringLiteral("عنصر عنصر عنصر"));
    QTextCursor cursor(editor.document()); cursor.setPosition(0); cursor.setPosition(4, QTextCursor::KeepAnchor); editor.setTextCursor(cursor);
    QTest::keyClick(&editor, Qt::Key_D, Qt::ControlModifier | Qt::AltModifier);
    QCOMPARE(editor.secondaryCursorCount(), 1);
    QTest::keyClick(&editor, Qt::Key_D, Qt::ControlModifier | Qt::AltModifier);
    QCOMPARE(editor.secondaryCursorCount(), 2);
    QKeyEvent firstArabicInput(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("س"));
    QCoreApplication::sendEvent(&editor, &firstArabicInput);
    QCOMPARE(editor.toPlainText(), QStringLiteral("س س س"));
    editor.undo();
    QCOMPARE(editor.toPlainText(), QStringLiteral("عنصر عنصر عنصر"));
    QCOMPARE(editor.secondaryCursorCount(), 0);

    cursor.setPosition(0); cursor.setPosition(4, QTextCursor::KeepAnchor); editor.setTextCursor(cursor);
    QTest::keyClick(&editor, Qt::Key_L, Qt::ControlModifier | Qt::ShiftModifier);
    QCOMPARE(editor.secondaryCursorCount(), 2);
    QKeyEvent secondArabicInput(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("ص"));
    QCoreApplication::sendEvent(&editor, &secondArabicInput);
    QCOMPARE(editor.toPlainText(), QStringLiteral("ص ص ص"));
}

void DockableToolsTest::multiCursorAddsVerticalCaretAndPreservesDuplicateLineCommand()
{
    TEditor editor; editor.resize(640, 300); editor.show(); editor.setPlainText(QStringLiteral("ا\nب\nج"));
    QTextCursor cursor(editor.document()); cursor.setPosition(0); editor.setTextCursor(cursor);
    QTest::keyClick(&editor, Qt::Key_Down, Qt::AltModifier | Qt::ShiftModifier);
    QCOMPARE(editor.secondaryCursorCount(), 1);
    QTest::keyClicks(&editor, QStringLiteral("X"));
    QCOMPARE(editor.toPlainText(), QStringLiteral("Xا\nXب\nج"));
    QTest::keyClick(&editor, Qt::Key_Escape);
    QCOMPARE(editor.secondaryCursorCount(), 0);

    editor.setPlainText(QStringLiteral("سطر")); cursor = editor.textCursor(); cursor.movePosition(QTextCursor::Start); editor.setTextCursor(cursor);
    editor.duplicateLine();
    QCOMPARE(editor.toPlainText(), QStringLiteral("سطر\nسطر"));
}

void DockableToolsTest::multiCursorControllerRetainsPrimaryAndDeduplicatesOverlaps()
{
    QTextDocument document(QStringLiteral("abcdef"));
    MultiCursorController controller(&document);
    QTextCursor primary(&document); primary.setPosition(1); primary.setPosition(4, QTextCursor::KeepAnchor);
    QTextCursor overlapping(&document); overlapping.setPosition(2);
    QVERIFY(!controller.toggleCursorAt(overlapping, primary));

    QTextCursor secondary(&document); secondary.setPosition(5);
    QVERIFY(controller.toggleCursorAt(secondary, primary));
    QCOMPARE(controller.secondaryCursorCount(), 1);
    QVERIFY(controller.toggleCursorAt(secondary, primary));
    QCOMPARE(controller.secondaryCursorCount(), 0);
    QVERIFY(controller.toggleCursorAt(secondary, primary));

    const QTextCursor editedPrimary = controller.insertText(primary, QStringLiteral("X"));
    QCOMPARE(document.toPlainText(), QStringLiteral("aXeXf"));
    QCOMPARE(editedPrimary.position(), 2);
    document.undo();
    QCOMPARE(document.toPlainText(), QStringLiteral("abcdef"));

    QTextDocument occurrenceDocument(QStringLiteral("عنصر عنصر عنصر"));
    MultiCursorController occurrenceController(&occurrenceDocument);
    QTextCursor occurrencePrimary(&occurrenceDocument); occurrencePrimary.setPosition(0); occurrencePrimary.setPosition(4, QTextCursor::KeepAnchor);
    QVERIFY(occurrenceController.selectNextOccurrence(occurrencePrimary));
    QCOMPARE(occurrenceController.secondaryCursorCount(), 1);
    QVERIFY(occurrenceController.selectNextOccurrence(occurrencePrimary));
    QCOMPARE(occurrenceController.secondaryCursorCount(), 2);

    QTextDocument deleteDocument(QStringLiteral("abcdef"));
    MultiCursorController deleteController(&deleteDocument);
    QTextCursor deletePrimary(&deleteDocument); deletePrimary.setPosition(1);
    QTextCursor deleteSecondary(&deleteDocument); deleteSecondary.setPosition(4);
    QVERIFY(deleteController.toggleCursorAt(deleteSecondary, deletePrimary));
    const QTextCursor deletedPrimary = deleteController.deleteForward(deletePrimary);
    QCOMPARE(deletedPrimary.position(), 1);
    QCOMPARE(deleteDocument.toPlainText(), QStringLiteral("acdf"));
    deleteDocument.undo();
    QCOMPARE(deleteDocument.toPlainText(), QStringLiteral("abcdef"));
}

void DockableToolsTest::multiCursorSupportsAltClickDeletionAndIndentedNewlines()
{
    TEditor editor; editor.resize(640, 300); editor.show(); editor.setFocus(); editor.setPlainText(QStringLiteral("ab\ncd"));
    QTextCursor primary(editor.document()); primary.setPosition(1); editor.setTextCursor(primary);
    QTextCursor secondary(editor.document()); secondary.setPosition(4);
    const QImage before = editor.viewport()->grab().toImage();
    QTest::mouseClick(editor.viewport(), Qt::LeftButton, Qt::AltModifier, editor.cursorRect(secondary).center());
    QCOMPARE(editor.secondaryCursorCount(), 1);
    QTextCursor third(editor.document()); third.setPosition(5);
    QTest::mouseClick(editor.viewport(), Qt::LeftButton, Qt::AltModifier, editor.cursorRect(third).center());
    QCOMPARE(editor.secondaryCursorCount(), 2);
    const QImage after = editor.viewport()->grab().toImage();
    QVERIFY(before != after);
    QTest::keyClick(&editor, Qt::Key_Escape);
    QCOMPARE(editor.secondaryCursorCount(), 0);

    editor.setPlainText(QStringLiteral("ab\ncd"));
    primary = QTextCursor(editor.document()); primary.setPosition(1); editor.setTextCursor(primary);
    secondary = QTextCursor(editor.document()); secondary.setPosition(4);
    QTest::mouseClick(editor.viewport(), Qt::LeftButton, Qt::AltModifier, editor.cursorRect(secondary).center());
    QCOMPARE(editor.secondaryCursorCount(), 1);
    QTest::keyClick(&editor, Qt::Key_Backspace);
    QCOMPARE(editor.toPlainText(), QStringLiteral("b\nd"));
    editor.undo();
    QCOMPARE(editor.toPlainText(), QStringLiteral("ab\ncd"));

    const QString firstLine = QStringLiteral("\tاذا:");
    editor.setPlainText(firstLine + QStringLiteral("\n\tاطبع"));
    primary = editor.textCursor(); primary.setPosition(firstLine.size()); editor.setTextCursor(primary);
    QTest::keyClick(&editor, Qt::Key_Down, Qt::AltModifier | Qt::ShiftModifier);
    QCOMPARE(editor.secondaryCursorCount(), 1);
    QTest::keyClick(&editor, Qt::Key_Return);
    QCOMPARE(editor.toPlainText(), QStringLiteral("\tاذا:\n\t\t\n\tاطبع\n\t"));
    editor.undo();
    QCOMPARE(editor.toPlainText(), firstLine + QStringLiteral("\n\tاطبع"));
}

void DockableToolsTest::applicationWindowControllerOwnsTopLevelWindowRouting()
{
    auto* const application = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(application != nullptr);
    ApplicationBootstrap::initialize(*application);

    ApplicationWindowController controller;
    int welcomeWindowCount = 0;
    WelcomeWindow* welcome = nullptr;
    Taif* editor = nullptr;
    connect(&controller, &ApplicationWindowController::welcomeWindowCreated,
            &controller, [&welcomeWindowCount, &welcome](WelcomeWindow* const window) {
                ++welcomeWindowCount;
                welcome = window;
            });
    connect(&controller, &ApplicationWindowController::editorWindowCreated,
            &controller, [&editor](Taif* const window) { editor = window; });

    controller.showInitial({});
    QTRY_COMPARE_WITH_TIMEOUT(welcomeWindowCount, 1, 1000);
    QVERIFY(welcome != nullptr);
    QVERIFY(welcome->isVisible());

    QMetaObject::invokeMethod(welcome, "newDocumentRequested", Qt::DirectConnection);
    QTRY_VERIFY_WITH_TIMEOUT(editor != nullptr, 1000);
    QVERIFY(editor->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!welcome->isVisible(), 1000);

    QMetaObject::invokeMethod(editor, "exitApp", Qt::DirectConnection);
    QTRY_COMPARE_WITH_TIMEOUT(welcomeWindowCount, 2, 3000);
    QVERIFY(welcome != nullptr);
    QVERIFY(welcome->isVisible());
    welcome->close();
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

void DockableToolsTest::editorServiceBindingsHaveIdempotentShutdown()
{
    QTextDocument document;
    EditorAnalysisBinding analysisBinding(&document);
    analysisBinding.initialize();
    QVERIFY(analysisBinding.controller() != nullptr);
    analysisBinding.shutdown();
    analysisBinding.shutdown();

    EditorInteractionBinding interactionBinding;
    int hoverTimeouts = 0;
    interactionBinding.initialize([&hoverTimeouts]() { ++hoverTimeouts; }, 1);
    interactionBinding.scheduleHover({}, 3, 7);
    QTRY_COMPARE_WITH_TIMEOUT(hoverTimeouts, 1, 1000);
    QVERIFY(interactionBinding.matches(3, 7));
    interactionBinding.dismissHover();
    QCOMPARE(interactionBinding.pendingOffset(), qsizetype(-1));
    interactionBinding.shutdown();
    interactionBinding.shutdown();

    EditorRecoveryBinding recoveryBinding(&document);
    recoveryBinding.initialize();
    recoveryBinding.shutdown();
    recoveryBinding.shutdown();
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

void DockableToolsTest::settingsRecentFileClearRemainsDraftLocalUntilApply()
{
    auto* const application = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(application != nullptr);
    ApplicationBootstrap::initialize(*application);

    QSettings persisted(QStringLiteral("Alif"), QStringLiteral("Taif"));
    const bool hadRecentFiles = persisted.contains(QStringLiteral("RecentFiles"));
    const QVariant originalRecentFiles = persisted.value(QStringLiteral("RecentFiles"));

    TSettings settings;
    auto* const clearButton = settings.findChild<QPushButton*>(
        QStringLiteral("SettingsClearRecentFilesButton"));
    auto* const cancelButton = settings.findChild<QPushButton*>(
        QStringLiteral("SettingsCancelButton"));
    QVERIFY(clearButton != nullptr);
    QVERIFY(cancelButton != nullptr);

    clearButton->click();
    QVERIFY(!clearButton->isEnabled());
    QCOMPARE(persisted.contains(QStringLiteral("RecentFiles")), hadRecentFiles);
    cancelButton->click();
    QVERIFY(persisted.contains(QStringLiteral("RecentFiles")) == hadRecentFiles);

    if (hadRecentFiles) {
        persisted.setValue(QStringLiteral("RecentFiles"), originalRecentFiles);
    } else {
        persisted.remove(QStringLiteral("RecentFiles"));
    }
    persisted.sync();
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
    QCOMPARE(viewActions.size(), 4);
    QCOMPARE(viewActions.at(0), menuBar.alifOutputAction);
    QCOMPARE(viewActions.at(1), menuBar.terminalAction);
    QCOMPARE(viewActions.at(2), menuBar.problemsAction);
    QCOMPARE(viewActions.at(3), menuBar.aiAssistantAction);
    QCOMPARE(menuBar.alifOutputAction->text(), QStringLiteral("مخرجات ألف"));
    QCOMPARE(menuBar.terminalAction->text(), QStringLiteral("الطرفية"));
    QCOMPARE(menuBar.problemsAction->text(), QStringLiteral("الأخطاء"));
    QCOMPARE(menuBar.aiAssistantAction->text(), QStringLiteral("مساعد الذكاء الاصطناعي"));
    QVERIFY(menuBar.alifOutputAction->isCheckable());
    QVERIFY(menuBar.terminalAction->isCheckable());
    QVERIFY(menuBar.problemsAction->isCheckable());
    QVERIFY(menuBar.aiAssistantAction->isCheckable());

    QSignalSpy alifOutputSpy(&menuBar, &TMenuBar::showAlifOutputRequested);
    QSignalSpy terminalSpy(&menuBar, &TMenuBar::showTerminalRequested);
    QSignalSpy problemsSpy(&menuBar, &TMenuBar::showProblemsRequested);
    QSignalSpy aiAssistantSpy(&menuBar, &TMenuBar::showAiAssistantRequested);

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

    menuBar.aiAssistantAction->trigger();
    QVERIFY(menuBar.aiAssistantAction->isChecked());

    menuBar.setOpenViewToolActions(false, true, true, false);
    QVERIFY(!menuBar.alifOutputAction->isChecked());
    QVERIFY(menuBar.terminalAction->isChecked());
    QVERIFY(menuBar.problemsAction->isChecked());
    QVERIFY(!menuBar.aiAssistantAction->isChecked());

    QCOMPARE(alifOutputSpy.count(), 1);
    QCOMPARE(terminalSpy.count(), 1);
    QCOMPARE(problemsSpy.count(), 1);
    QCOMPARE(aiAssistantSpy.count(), 1);
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

void DockableToolsTest::sessionStoreMigratesLegacyDataAndRecoversFromBackup()
{
    const QString settingsFile = QDir(QDir::tempPath()).filePath(
        QStringLiteral("taif-session-migration-%1.ini")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    const SessionStore::SettingsScope scope{
        QStringLiteral("TaifEditorSessionStoreTests"),
        QStringLiteral("MigrationScope"), settingsFile};

    SavedSession legacySession;
    legacySession.id = QStringLiteral("legacy-session-01");
    legacySession.displayName = QStringLiteral("جلسة قديمة");
    legacySession.filePaths = {QDir::tempPath() + QStringLiteral("/legacy-session.alif")};
    legacySession.activeFilePath = legacySession.filePaths.first();
    legacySession.updatedAt = QDateTime::currentDateTimeUtc();

    QSettings legacySettings(settingsFile, QSettings::IniFormat);
    legacySettings.beginGroup(QStringLiteral("SavedSessions"));
    QVariantMap legacyEntry;
    legacyEntry.insert(QStringLiteral("id"), legacySession.id);
    legacyEntry.insert(QStringLiteral("displayName"), legacySession.displayName);
    legacyEntry.insert(QStringLiteral("filePaths"), legacySession.filePaths);
    legacyEntry.insert(QStringLiteral("activeFilePath"), legacySession.activeFilePath);
    legacyEntry.insert(QStringLiteral("updatedAt"), legacySession.updatedAt.toString(Qt::ISODateWithMs));
    legacySettings.setValue(QStringLiteral("schemaVersion"), 1);
    legacySettings.setValue(QStringLiteral("entries"), QVariantList{legacyEntry});
    legacySettings.endGroup();
    legacySettings.sync();

    SessionStore store(scope);
    QString errorMessage;
    const QVector<SavedSession> migrated = store.loadAll(&errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(migrated.size(), 1);
    QCOMPARE(migrated.first().id, legacySession.id);
    QVERIFY(QFileInfo::exists(store.repositoryFilePath()));
    QVERIFY(QFileInfo::exists(settingsFile));

    SavedSession updated = migrated.first();
    updated.displayName = QStringLiteral("جلسة محدثة");
    QVERIFY(store.update(updated, &errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(QFileInfo::exists(store.backupRepositoryFilePath()));

    QFile corruptPrimary(store.repositoryFilePath());
    QVERIFY(corruptPrimary.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(corruptPrimary.write("not-json"), qint64(8));
    corruptPrimary.close();

    const QVector<SavedSession> recovered = store.loadAll(&errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(recovered.size(), 1);
    QCOMPARE(recovered.first().displayName, legacySession.displayName);

    QFile::remove(settingsFile);
    QFile::remove(store.repositoryFilePath());
    QFile::remove(store.backupRepositoryFilePath());
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
