#include "ApplicationBootstrap.h"
#include "Taif.h"
#include "TWelcomeWindow.h"

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
    if (!launchRequest.filePath.isEmpty()) {
        auto* const editor = new Taif(launchRequest.filePath);
        editor->show();
    } else {
        auto* const welcomeWindow = new WelcomeWindow();
        welcomeWindow->show();
    }

    return application.exec();
}
