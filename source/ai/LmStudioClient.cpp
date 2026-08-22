#include "LmStudioClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {
constexpr auto kJsonContentType = "application/json";
constexpr auto kEventStreamContentType = "text/event-stream";
constexpr auto kAssistantRole = "assistant";
constexpr auto kUserRole = "user";
constexpr auto kSystemRole = "system";
constexpr auto kToolRole = "tool";

QString roleName(const AiChatRole role)
{
    switch (role) {
    case AiChatRole::System: return QString::fromLatin1(kSystemRole);
    case AiChatRole::User: return QString::fromLatin1(kUserRole);
    case AiChatRole::Assistant: return QString::fromLatin1(kAssistantRole);
    case AiChatRole::Tool: return QString::fromLatin1(kToolRole);
    }
    return QString::fromLatin1(kUserRole);
}
}

LmStudioClient::LmStudioClient(QObject* const parent)
    : QObject(parent)
    , m_settings(AiAssistantSettingsStore::load())
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
        if (m_activeReply != nullptr) {
            m_activeReply->abort();
            failRequest(QStringLiteral("انتهت مهلة الاتصال بخادم LM Studio."), 0, true);
        }
    });
}

LmStudioClient::~LmStudioClient()
{
    cancelActiveRequest();
}

void LmStudioClient::setSettings(const AiAssistantSettings& settingsValue)
{
    m_settings = AiAssistantSettingsStore::normalize(settingsValue);
}

AiAssistantSettings LmStudioClient::settings() const
{
    return m_settings;
}

AiConnectionState LmStudioClient::state() const
{
    return m_state;
}

bool LmStudioClient::hasActiveRequest() const
{
    return m_activeReply != nullptr;
}

void LmStudioClient::discoverModels()
{
    ensureNetwork();
    cancelActiveRequest();
    setState(AiConnectionState::DiscoveringModels);
    QNetworkRequest request(endpoint(QStringLiteral("models")));
    request.setRawHeader("Accept", kJsonContentType);
    QNetworkReply* const reply = m_network->get(request);
    beginRequest(reply, RequestKind::Models);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply != m_activeReply) {
            reply->deleteLater();
            return;
        }
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            const QString message = reply->errorString();
            reply->deleteLater();
            failRequest(QStringLiteral("تعذر الاتصال بـ LM Studio: %1").arg(message), status, true);
            return;
        }
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray data = document.object().value(QStringLiteral("data")).toArray();
        QVector<AiModelDescriptor> models;
        models.reserve(data.size());
        for (const QJsonValue& value : data) {
            const QJsonObject object = value.toObject();
            AiModelDescriptor model;
            model.id = object.value(QStringLiteral("id")).toString();
            model.displayName = object.value(QStringLiteral("display_name")).toString(model.id);
            model.ownedBy = object.value(QStringLiteral("owned_by")).toString();
            if (model.isValid()) {
                models.append(model);
            }
        }
        reply->deleteLater();
        finishRequest();
        emit modelsReceived(models);
    });
}

void LmStudioClient::streamChat(const QVector<AiChatMessage>& messages, const QString& modelId,
                                const QJsonArray& tools)
{
    ensureNetwork();
    cancelActiveRequest();
    if (modelId.trimmed().isEmpty()) {
        failRequest(QStringLiteral("اختر نموذجاً محملاً في LM Studio أولاً."));
        return;
    }

    QJsonArray serializedMessages;
    for (const AiChatMessage& message : messages) {
        serializedMessages.append(messageObject(message));
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("model"), modelId);
    payload.insert(QStringLiteral("messages"), serializedMessages);
    payload.insert(QStringLiteral("stream"), true);
    payload.insert(QStringLiteral("stream_options"), QJsonObject{{QStringLiteral("include_usage"), true}});
    if (!tools.isEmpty()) {
        payload.insert(QStringLiteral("tools"), tools);
        payload.insert(QStringLiteral("tool_choice"), QStringLiteral("auto"));
    }

    QNetworkRequest request(endpoint(QStringLiteral("chat/completions")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QString::fromLatin1(kJsonContentType));
    request.setRawHeader("Accept", kEventStreamContentType);
    QNetworkReply* const reply = m_network->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    setState(AiConnectionState::Streaming);
    beginRequest(reply, RequestKind::Chat);

    connect(reply, &QNetworkReply::readyRead, this, &LmStudioClient::consumeStreamBytes);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply != m_activeReply) {
            reply->deleteLater();
            return;
        }
        consumeStreamBytes();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && m_state != AiConnectionState::Cancelling) {
            const QByteArray body = reply->readAll().left(2048);
            const QString detail = QString::fromUtf8(body).trimmed();
            const QString message = detail.isEmpty() ? reply->errorString() : detail;
            reply->deleteLater();
            failRequest(QStringLiteral("فشلت استجابة LM Studio: %1").arg(message), status, status >= 500 || status == 0);
            return;
        }
        reply->deleteLater();
        const bool wasCancelling = m_state == AiConnectionState::Cancelling;
        finishRequest();
        emit streamFinished(wasCancelling ? QStringLiteral("cancelled") : QStringLiteral("stop"));
    });
}

void LmStudioClient::cancelActiveRequest()
{
    if (m_activeReply == nullptr) {
        return;
    }
    setState(AiConnectionState::Cancelling);
    m_timeout.stop();
    m_activeReply->abort();
}

void LmStudioClient::ensureNetwork()
{
    if (m_network == nullptr) {
        m_network = new QNetworkAccessManager(this);
    }
}

void LmStudioClient::setState(const AiConnectionState stateValue)
{
    if (m_state == stateValue) {
        return;
    }
    m_state = stateValue;
    emit stateChanged(m_state);
}

void LmStudioClient::beginRequest(QNetworkReply* const reply, const RequestKind kind)
{
    m_activeReply = reply;
    m_requestKind = kind;
    m_streamBuffer.clear();
    m_receivedCharacters = 0;
    m_toolArgumentBuffers.clear();
    m_toolNames.clear();
    m_toolIdsByIndex.clear();
    m_timeout.start(m_settings.requestTimeoutMilliseconds);
}

void LmStudioClient::renewInactivityDeadline()
{
    if (m_activeReply != nullptr) {
        m_timeout.start(m_settings.requestTimeoutMilliseconds);
    }
}

void LmStudioClient::finishRequest()
{
    m_timeout.stop();
    m_activeReply = nullptr;
    m_requestKind = RequestKind::None;
    m_streamBuffer.clear();
    m_toolArgumentBuffers.clear();
    m_toolNames.clear();
    m_toolIdsByIndex.clear();
    setState(AiConnectionState::Ready);
}

void LmStudioClient::failRequest(const QString& message, const int httpStatus, const bool retryable)
{
    m_timeout.stop();
    m_activeReply = nullptr;
    m_requestKind = RequestKind::None;
    m_streamBuffer.clear();
    setState(AiConnectionState::Error);
    emit requestFailed({message, httpStatus, retryable});
}

void LmStudioClient::consumeStreamBytes()
{
    if (m_activeReply == nullptr || m_requestKind != RequestKind::Chat) {
        return;
    }
    m_streamBuffer.append(m_activeReply->readAll());
    while (true) {
        const int newline = m_streamBuffer.indexOf('\n');
        if (newline < 0) {
            break;
        }
        QByteArray line = m_streamBuffer.left(newline);
        m_streamBuffer.remove(0, newline + 1);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        consumeSseLine(line);
    }
    if (m_streamBuffer.size() > m_settings.maximumResponseCharacters * 4) {
        if (m_activeReply != nullptr) {
            m_activeReply->abort();
        }
        failRequest(QStringLiteral("تجاوزت استجابة النموذج الحد المسموح به."));
    }
}

void LmStudioClient::consumeSseLine(const QByteArray& line)
{
    if (!line.startsWith("data:")) {
        return;
    }
    const QByteArray payload = line.mid(5).trimmed();
    if (payload == "[DONE]") {
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(payload);
    const QJsonObject root = document.object();
    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        return;
    }
    renewInactivityDeadline();
    const QJsonObject choice = choices.first().toObject();
    const QJsonObject delta = choice.value(QStringLiteral("delta")).toObject();
    const QString content = delta.value(QStringLiteral("content")).toString();
    if (!content.isEmpty()) {
        m_receivedCharacters += content.size();
        if (m_receivedCharacters > m_settings.maximumResponseCharacters) {
            if (m_activeReply != nullptr) {
                m_activeReply->abort();
            }
            failRequest(QStringLiteral("تجاوزت استجابة النموذج الحد المسموح به."));
            return;
        }
        emit streamDelta(content);
    }

    const QJsonArray toolCalls = delta.value(QStringLiteral("tool_calls")).toArray();
    for (const QJsonValue& value : toolCalls) {
        const QJsonObject call = value.toObject();
        const int index = call.value(QStringLiteral("index")).toInt();
        const QString id = call.value(QStringLiteral("id")).toString();
        const QJsonObject function = call.value(QStringLiteral("function")).toObject();
        const QString name = function.value(QStringLiteral("name")).toString();
        const QString fragment = function.value(QStringLiteral("arguments")).toString();
        const QString key = !id.isEmpty() ? id : m_toolIdsByIndex.value(index, QString::number(index));
        if (!id.isEmpty()) {
            m_toolIdsByIndex.insert(index, id);
        }
        if (!name.isEmpty()) {
            m_toolNames.insert(key, name);
        }
        if (!fragment.isEmpty()) {
            m_toolArgumentBuffers[key].append(fragment);
        }
        emit toolCallDelta(key, m_toolNames.value(key), fragment);
    }
}

QUrl LmStudioClient::endpoint(const QString& suffix) const
{
    QUrl url(m_settings.endpointUrl + QLatin1Char('/') + suffix);
    return url;
}

QJsonObject LmStudioClient::messageObject(const AiChatMessage& message) const
{
    QJsonObject result;
    result.insert(QStringLiteral("role"), roleName(message.role));
    result.insert(QStringLiteral("content"), message.content);
    if (!message.toolCalls.isEmpty()) {
        result.insert(QStringLiteral("tool_calls"), message.toolCalls);
    }
    if (!message.toolCallId.isEmpty()) {
        result.insert(QStringLiteral("tool_call_id"), message.toolCallId);
    }
    if (!message.name.isEmpty()) {
        result.insert(QStringLiteral("name"), message.name);
    }
    return result;
}
