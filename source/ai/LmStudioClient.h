#pragma once

#include "AiAgentTypes.h"
#include "AiAssistantSettings.h"

#include <QJsonArray>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;

/** Qt Network client for LM Studio's OpenAI-compatible local server endpoints. */
class LmStudioClient final : public QObject {
    Q_OBJECT
public:
    explicit LmStudioClient(QObject* parent = nullptr);
    ~LmStudioClient() override;

    void setSettings(const AiAssistantSettings& settings);
    [[nodiscard]] AiAssistantSettings settings() const;
    [[nodiscard]] AiConnectionState state() const;
    [[nodiscard]] bool hasActiveRequest() const;

    void discoverModels();
    void streamChat(const QVector<AiChatMessage>& messages, const QString& modelId,
                    const QJsonArray& tools = {});
    void cancelActiveRequest();

signals:
    void stateChanged(AiConnectionState state);
    void modelsReceived(QVector<AiModelDescriptor> models);
    void streamDelta(QString text);
    void toolCallDelta(QString id, QString name, QString argumentsFragment);
    void streamFinished(QString finishReason);
    void requestFailed(AiTransportError error);

private:
    enum class RequestKind : quint8 { None, Models, Chat };

    void ensureNetwork();
    void setState(AiConnectionState state);
    void beginRequest(QNetworkReply* reply, RequestKind kind);
    void renewInactivityDeadline();
    void finishRequest();
    void failRequest(const QString& message, int httpStatus = 0, bool retryable = false);
    void consumeStreamBytes();
    void consumeSseLine(const QByteArray& line);
    [[nodiscard]] QUrl endpoint(const QString& suffix) const;
    [[nodiscard]] QJsonObject messageObject(const AiChatMessage& message) const;

    QNetworkAccessManager* m_network = nullptr;
    AiAssistantSettings m_settings;
    QPointer<QNetworkReply> m_activeReply;
    RequestKind m_requestKind = RequestKind::None;
    AiConnectionState m_state = AiConnectionState::Offline;
    QTimer m_timeout;
    QByteArray m_streamBuffer;
    qsizetype m_receivedCharacters = 0;
    QHash<QString, QString> m_toolArgumentBuffers;
    QHash<QString, QString> m_toolNames;
    QHash<int, QString> m_toolIdsByIndex;
};
