#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>

#include <QDir>
#include <QTemporaryDir>
#include <QTimer>

#include "AlifRunController.h"
#include "OutputBuffer.h"
#include "RecoveryCoordinator.h"

class ControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void outputBufferBoundsAndAccountsForDroppedChunks();
    void recoveryFlushIsAsynchronousAndCompletes();
    void alifRunControllerCompletesOnceForQuickProcess();
};

void ControllerTests::outputBufferBoundsAndAccountsForDroppedChunks()
{
    OutputBuffer::Limits limits;
    limits.maximumPendingBytes = 128;
    limits.maximumChunkBytes = 64;
    OutputBuffer buffer(limits);

    buffer.append(QString(32, QLatin1Char('A')));
    buffer.append(QString(32, QLatin1Char('B')));
    buffer.append(QString(32, QLatin1Char('C')));

    const OutputBuffer::DrainResult result = buffer.drain();
    QVERIFY(result.truncated);
    QCOMPARE(result.droppedBytes, qsizetype(64));
    QCOMPARE(result.text, QString(32, QLatin1Char('B')) + QString(32, QLatin1Char('C')));
    QCOMPARE(buffer.pendingBytes(), qsizetype(0));
}

void ControllerTests::recoveryFlushIsAsynchronousAndCompletes()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    RecoveryCoordinator coordinator(directory.filePath(QStringLiteral("recovery")));
    QSignalSpy flushSpy(&coordinator, &RecoveryCoordinator::flushCompleted);

    bool queuedCallbackRan = false;
    QTimer::singleShot(0, &coordinator, [&queuedCallbackRan]() { queuedCallbackRan = true; });
    coordinator.requestFlush(250);

    QVERIFY(!queuedCallbackRan);
    QVERIFY(flushSpy.isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(flushSpy.count(), 1, 1000);
    QVERIFY(flushSpy.constFirst().constFirst().toBool());
}

void ControllerTests::alifRunControllerCompletesOnceForQuickProcess()
{
    AlifRunController controller;
    AlifRunController::Request request;
#if defined(Q_OS_WIN)
    request.program = qEnvironmentVariable("COMSPEC", QStringLiteral("C:/Windows/System32/cmd.exe"));
    request.arguments = {QStringLiteral("/C"), QStringLiteral("echo controller-target")};
#else
    request.program = QStringLiteral("/bin/sh");
    request.arguments = {QStringLiteral("-c"), QStringLiteral("printf controller-target")};
#endif
    request.workingDirectory = QDir::tempPath();

    QSignalSpy outputSpy(&controller, &AlifRunController::standardOutput);
    QSignalSpy finishedSpy(&controller, &AlifRunController::finished);
    QString errorMessage;
    QVERIFY(controller.start(request, &errorMessage));
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3000);
    QVERIFY(!outputSpy.isEmpty());
    QVERIFY(outputSpy.constFirst().constFirst().toString().contains(
        QStringLiteral("controller-target")));
    QVERIFY(!controller.isActive());
}

QTEST_MAIN(ControllerTests)
#include "tst_Controllers.moc"
