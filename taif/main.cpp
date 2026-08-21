#include "ApplicationBootstrap.h"
#include "ApplicationWindowController.h"

#include <QApplication>
#include <QMessageBox>

#include <cstdlib>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    ApplicationBootstrap::initialize(application);

    const ApplicationLaunchRequest launchRequest =
        ApplicationBootstrap::parseLaunchRequest(application.arguments());
    if (!launchRequest.isValid()) {
        QMessageBox::warning(nullptr, QStringLiteral("طيف"), launchRequest.errorMessage);
        return EXIT_FAILURE;
    }

    application.setQuitOnLastWindowClosed(true);
    ApplicationWindowController windowController(&application);
    windowController.showInitial(launchRequest);

    return application.exec();
}
