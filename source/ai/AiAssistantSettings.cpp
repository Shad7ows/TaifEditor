#include "AiAssistantSettings.h"

#include <QSettings>
#include <QUrl>

namespace {
constexpr auto kSettingsGroup = "AiAssistant";
constexpr int kMinimumRequestTimeoutMilliseconds = 30000;
constexpr int kMaximumRequestTimeoutMilliseconds = 3600000;
constexpr int kMinimumCommandTimeoutMilliseconds = 5000;
constexpr int kMaximumCommandTimeoutMilliseconds = 1800000;
constexpr int kMinimumResponseCharacters = 4096;
constexpr int kMaximumResponseCharacters = 1000000;
constexpr int kMinimumContextCharacters = 1024;
constexpr int kMaximumContextCharacters = 250000;

QString normalizedEndpoint(QString endpoint)
{
    endpoint = endpoint.trimmed();
    if (endpoint.isEmpty()) {
        return AiAssistantSettings{}.endpointUrl;
    }
    QUrl url(endpoint);
    if (!url.isValid() || url.scheme().isEmpty() || url.host().isEmpty()) {
        return AiAssistantSettings{}.endpointUrl;
    }
    if (url.path().isEmpty() || url.path() == QStringLiteral("/")) {
        url.setPath(QStringLiteral("/v1"));
    }
    url.setQuery(QString());
    url.setFragment(QString());
    QString normalized = url.toString(QUrl::RemovePassword | QUrl::RemoveUserInfo);
    while (normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    return normalized;
}
}

bool AiAssistantSettings::operator==(const AiAssistantSettings& other) const
{
    return endpointUrl == other.endpointUrl && preferredModel == other.preferredModel
        && requestTimeoutMilliseconds == other.requestTimeoutMilliseconds
        && commandTimeoutMilliseconds == other.commandTimeoutMilliseconds
        && autonomyMode == other.autonomyMode
        && maximumResponseCharacters == other.maximumResponseCharacters
        && maximumFileContextCharacters == other.maximumFileContextCharacters
        && maximumToolOutputCharacters == other.maximumToolOutputCharacters
        && remoteEndpointAcknowledged == other.remoteEndpointAcknowledged;
}

AiAssistantSettings AiAssistantSettingsStore::defaults()
{
    return {};
}

AiAssistantSettings AiAssistantSettingsStore::normalize(AiAssistantSettings settings)
{
    settings.endpointUrl = normalizedEndpoint(std::move(settings.endpointUrl));
    settings.preferredModel = settings.preferredModel.trimmed();
    settings.requestTimeoutMilliseconds = qBound(kMinimumRequestTimeoutMilliseconds,
                                                 settings.requestTimeoutMilliseconds,
                                                 kMaximumRequestTimeoutMilliseconds);
    settings.commandTimeoutMilliseconds = qBound(kMinimumCommandTimeoutMilliseconds,
                                                 settings.commandTimeoutMilliseconds,
                                                 kMaximumCommandTimeoutMilliseconds);
    if (settings.autonomyMode != AiAutonomyMode::Manual
        && settings.autonomyMode != AiAutonomyMode::WorkspaceAuto) {
        settings.autonomyMode = AiAutonomyMode::WorkspaceAuto;
    }
    settings.maximumResponseCharacters = qBound(kMinimumResponseCharacters,
                                                settings.maximumResponseCharacters,
                                                kMaximumResponseCharacters);
    settings.maximumFileContextCharacters = qBound(kMinimumContextCharacters,
                                                   settings.maximumFileContextCharacters,
                                                   kMaximumContextCharacters);
    settings.maximumToolOutputCharacters = qBound(kMinimumContextCharacters,
                                                  settings.maximumToolOutputCharacters,
                                                  kMaximumContextCharacters);
    if (isLoopbackEndpoint(settings.endpointUrl)) {
        settings.remoteEndpointAcknowledged = false;
    }
    return settings;
}

bool AiAssistantSettingsStore::isLoopbackEndpoint(const QString& endpointUrl)
{
    const QUrl url(endpointUrl);
    const QString host = url.host().toLower();
    return host == QStringLiteral("127.0.0.1") || host == QStringLiteral("localhost")
        || host == QStringLiteral("::1");
}

AiAssistantSettings AiAssistantSettingsStore::load()
{
    QSettings settings(QStringLiteral("Alif"), QStringLiteral("Taif"));
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    AiAssistantSettings result;
    result.endpointUrl = settings.value(QStringLiteral("endpointUrl"), result.endpointUrl).toString();
    result.preferredModel = settings.value(QStringLiteral("preferredModel")).toString();
    result.requestTimeoutMilliseconds = settings.value(QStringLiteral("requestTimeoutMilliseconds"),
                                                        result.requestTimeoutMilliseconds).toInt();
    result.commandTimeoutMilliseconds = settings.value(QStringLiteral("commandTimeoutMilliseconds"),
                                                        result.commandTimeoutMilliseconds).toInt();
    result.autonomyMode = static_cast<AiAutonomyMode>(settings.value(QStringLiteral("autonomyMode"),
                                                                       static_cast<int>(result.autonomyMode)).toInt());
    result.maximumResponseCharacters = settings.value(QStringLiteral("maximumResponseCharacters"),
                                                       result.maximumResponseCharacters).toInt();
    result.maximumFileContextCharacters = settings.value(QStringLiteral("maximumFileContextCharacters"),
                                                          result.maximumFileContextCharacters).toInt();
    result.maximumToolOutputCharacters = settings.value(QStringLiteral("maximumToolOutputCharacters"),
                                                        result.maximumToolOutputCharacters).toInt();
    result.remoteEndpointAcknowledged = settings.value(QStringLiteral("remoteEndpointAcknowledged"), false).toBool();
    settings.endGroup();
    return normalize(std::move(result));
}

bool AiAssistantSettingsStore::save(const AiAssistantSettings& settingsToSave, QString* const errorMessage)
{
    const AiAssistantSettings settingsValue = normalize(settingsToSave);
    if (!isLoopbackEndpoint(settingsValue.endpointUrl) && !settingsValue.remoteEndpointAcknowledged) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("يتطلب الاتصال بخادم غير محلي تأكيد الخصوصية.");
        }
        return false;
    }

    QSettings settings(QStringLiteral("Alif"), QStringLiteral("Taif"));
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    settings.setValue(QStringLiteral("schemaVersion"), kCurrentSchemaVersion);
    settings.setValue(QStringLiteral("endpointUrl"), settingsValue.endpointUrl);
    settings.setValue(QStringLiteral("preferredModel"), settingsValue.preferredModel);
    settings.setValue(QStringLiteral("requestTimeoutMilliseconds"), settingsValue.requestTimeoutMilliseconds);
    settings.setValue(QStringLiteral("commandTimeoutMilliseconds"), settingsValue.commandTimeoutMilliseconds);
    settings.setValue(QStringLiteral("autonomyMode"), static_cast<int>(settingsValue.autonomyMode));
    settings.setValue(QStringLiteral("maximumResponseCharacters"), settingsValue.maximumResponseCharacters);
    settings.setValue(QStringLiteral("maximumFileContextCharacters"), settingsValue.maximumFileContextCharacters);
    settings.setValue(QStringLiteral("maximumToolOutputCharacters"), settingsValue.maximumToolOutputCharacters);
    settings.setValue(QStringLiteral("remoteEndpointAcknowledged"), settingsValue.remoteEndpointAcknowledged);
    settings.endGroup();
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("تعذر حفظ إعدادات مساعد الذكاء الاصطناعي.");
        }
        return false;
    }
    return true;
}
