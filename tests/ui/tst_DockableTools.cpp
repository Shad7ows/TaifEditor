#include <QtTest/QTest>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMainWindow>

#include "DockableConsoleTool.h"
#include "TConsole.h"

class DockableToolsTest final : public QObject {
    Q_OBJECT

private slots:
    void bottomToolsArePersistentAndTabified();
    void activationSelectsRequestedBottomToolTab();
};

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

QTEST_MAIN(DockableToolsTest)
#include "tst_DockableTools.moc"
