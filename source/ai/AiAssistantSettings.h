#pragma once

#include "AiAgentTypes.h"

#include <QString>

/** Persistent, non-secret configuration for the local AI assistant. */
struct AiAssistantSettings final {
    QString endpointUrl = QStringLiteral("http://127.0.0.1:1234/v1");
    QString preferredModel;
    // Local models can take minutes to load, reason, or begin emitting a stream.
    int requestTimeoutMilliseconds = 600000;
    int commandTimeoutMilliseconds = 300000;
    AiAutonomyMode autonomyMode = AiAutonomyMode::WorkspaceAuto;
    int maximumResponseCharacters = 200000;
    int maximumFileContextCharacters = 30000;
    int maximumToolOutputCharacters = 40000;
    bool remoteEndpointAcknowledged = false;

    [[nodiscard]] bool operator==(const AiAssistantSettings& other) const;
    [[nodiscard]] bool operator!=(const AiAssistantSettings& other) const { return !(*this == other); }
};

class AiAssistantSettingsStore final {
public:
    static constexpr int kCurrentSchemaVersion = 2;

    [[nodiscard]] static AiAssistantSettings defaults();
    [[nodiscard]] static AiAssistantSettings load();
    [[nodiscard]] static AiAssistantSettings normalize(AiAssistantSettings settings);
    [[nodiscard]] static bool isLoopbackEndpoint(const QString& endpointUrl);
    static bool save(const AiAssistantSettings& settings, QString* errorMessage = nullptr);

private:
    AiAssistantSettingsStore() = delete;
};
