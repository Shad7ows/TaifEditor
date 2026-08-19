#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMainWindow>

#include "DockableConsoleTool.h"
#include "TConsole.h"
#include "TMenu.h"

class DockableToolsTest final : public QObject {
    Q_OBJECT

private slots:
    void bottomToolsArePersistentAndTabified();
    void activationSelectsRequestedBottomToolTab();
    void viewMenuExposesOrderedDockActions();
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

QTEST_MAIN(DockableToolsTest)
#include "tst_DockableTools.moc"
