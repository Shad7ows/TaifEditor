#include "AiWorkspacePolicy.h"

#include <QRegularExpression>
#include <QStringList>

namespace {

bool deny(QString* const reason, const QString& message)
{
    if (reason != nullptr) {
        *reason = message;
    }
    return true;
}

bool startsWithAny(const QString& value, const QStringList& prefixes)
{
    for (const QString& prefix : prefixes) {
        if (value == prefix || value.startsWith(prefix + QLatin1Char(' '))) {
            return true;
        }
    }
    return false;
}

} // namespace

namespace AiWorkspacePolicy {

bool commandRequiresApproval(const QString& command, QString* const reason)
{
    if (reason != nullptr) {
        reason->clear();
    }

    const QString normalized = command.simplified();
    if (normalized.isEmpty()) {
        return deny(reason, QStringLiteral("الأمر فارغ."));
    }
    if (normalized.contains(QRegularExpression(QStringLiteral("[|;&><`$%\\n\\r\\\\]")))) {
        return deny(reason, QStringLiteral("الأوامر المركبة أو التوسعات أو إعادة التوجيه تحتاج مراجعة."));
    }

    const QString lower = normalized.toLower();
    const QString executable = lower.section(QLatin1Char(' '), 0, 0);
    if (executable == QStringLiteral("qmake")) {
        if (lower == QStringLiteral("qmake")) {
            return false;
        }
        return deny(reason, QStringLiteral("يسمح التنفيذ التلقائي لـ qmake دون خيارات فقط."));
    }
    if (executable == QStringLiteral("nmake") || executable == QStringLiteral("make")) {
        if (lower == executable) {
            return false;
        }
        return deny(reason, QStringLiteral("يسمح التنفيذ التلقائي بأمر البناء المحلي البسيط فقط."));
    }
    if (executable == QStringLiteral("cmake")) {
        if (lower.startsWith(QStringLiteral("cmake --build "))
            && !lower.contains(QStringLiteral("--target install"))
            && !lower.contains(QStringLiteral("--target package"))) {
            return false;
        }
        return deny(reason, QStringLiteral("يسمح التنفيذ التلقائي بـ cmake --build للمشروع المحلي فقط."));
    }
    if (executable == QStringLiteral("ctest")) {
        if (lower == QStringLiteral("ctest") || lower.startsWith(QStringLiteral("ctest --test-dir "))
            || lower.startsWith(QStringLiteral("ctest -c "))) {
            return false;
        }
        return deny(reason, QStringLiteral("يسمح التنفيذ التلقائي باختبارات CTest المحلية فقط."));
    }
    if (executable == QStringLiteral("git")) {
        if (startsWithAny(lower, {QStringLiteral("git status"), QStringLiteral("git diff"),
                                  QStringLiteral("git log"), QStringLiteral("git branch")})) {
            return false;
        }
        return deny(reason, QStringLiteral("يسمح التنفيذ التلقائي باستعلامات Git المحلية للقراءة فقط."));
    }

    return deny(reason, QStringLiteral("الأمر ليس ضمن قائمة البناء أو الاختبار أو الاستعلام المحلي الآمنة."));
}

} // namespace AiWorkspacePolicy
