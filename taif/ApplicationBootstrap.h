#pragma once

#include <QString>
#include <QStringList>

class QApplication;

struct ApplicationLaunchRequest final {
    QString filePath;
    QString errorMessage;

    [[nodiscard]] bool isValid() const { return errorMessage.isEmpty(); }
};

/**
 * Process-wide startup policy for TaifEditor. This class must be initialized
 * after QApplication construction and before constructing any application UI.
 */
class ApplicationBootstrap final {
public:
    static void initialize(QApplication& application);

    [[nodiscard]] static QString uiArabicFamily();
    [[nodiscard]] static QString displayArabicFamily();
    [[nodiscard]] static QString codeMonospaceFamily();
    [[nodiscard]] static QStringList availableEditorFontFamilies();
    [[nodiscard]] static ApplicationLaunchRequest parseLaunchRequest(
        const QStringList& arguments);

private:
    ApplicationBootstrap() = delete;
};
