#include "Taif.h"
#include "TWelcomeWindow.h"
#include "TaifBootstrap.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    TaifBootstrap::initialize(application);

    const TaifLaunchRequest launchRequest =
        TaifBootstrap::parseLaunchRequest(application.arguments());
    if (!launchRequest.isValid()) {
        QMessageBox::warning(nullptr, QStringLiteral("طيف"), launchRequest.errorMessage);
        return EXIT_FAILURE;
    }

    // هذه الدالة تسبب إغلاق البرنامج بشكل غير صحيح عند الضغط على خروج في قائمة ملف
    // application.setQuitOnLastWindowClosed(true); //* review
    if (!launchRequest.filePath.isEmpty()) {
        Taif* const editor = new Taif(launchRequest.filePath);

        editor->show();
    } else {
        WelcomeWindow* const welcomeWindow = new WelcomeWindow();
        welcomeWindow->show();
    }

    return application.exec();
}