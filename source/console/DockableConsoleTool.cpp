#include "DockableConsoleTool.h"

#include "TConsole.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QTimer>

DockableConsoleTool DockableConsoleToolFactory::create(QMainWindow* const host,
                                                        const QString& title,
                                                        const QString& dockObjectName,
                                                        const QString& consoleObjectName,
                                                        const bool startSystemShell)
{
    if (host == nullptr) {
        return {};
    }

    auto* dock = new QDockWidget(title, host);
    dock->setObjectName(dockObjectName);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetMovable
                      | QDockWidget::DockWidgetFloatable
                      | QDockWidget::DockWidgetClosable);

    auto* console = new TConsole(dock);
    console->setObjectName(consoleObjectName);
    console->setConsoleRTL();
    dock->setWidget(console);
    host->addDockWidget(Qt::BottomDockWidgetArea, dock);

    if (startSystemShell) {
        console->startCmd();
    }

    return {dock, console};
}

void DockableConsoleToolFactory::ensureTabifiedWith(QMainWindow* const host,
                                                    QDockWidget* const anchor,
                                                    QDockWidget* const dock)
{
    if (host == nullptr || anchor == nullptr || dock == nullptr || anchor == dock) {
        return;
    }

    if (anchor->isFloating() || dock->isFloating()
        || host->dockWidgetArea(anchor) != Qt::BottomDockWidgetArea
        || host->dockWidgetArea(dock) != Qt::BottomDockWidgetArea) {
        return;
    }

    if (!host->tabifiedDockWidgets(anchor).contains(dock)) {
        host->tabifyDockWidget(anchor, dock);
    }
}

bool DockableConsoleToolFactory::isRenderedTab(const QDockWidget* const dock)
{
    return dock != nullptr && dock->isVisible() && !dock->visibleRegion().isEmpty();
}

void DockableConsoleToolFactory::showAndActivate(QDockWidget* const dock)
{
    if (dock == nullptr) {
        return;
    }

    const auto activateDock = [dock]() {
        if (!dock->isVisible()) {
            return;
        }
        dock->raise();
        if (QWidget* const tool = dock->widget()) {
            tool->setFocus(Qt::OtherFocusReason);
        }
    };

    dock->show();
    activateDock();
    QTimer::singleShot(0, dock, activateDock);
}
