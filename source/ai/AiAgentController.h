#pragma once

#include "AiAgentTypes.h"

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QVector>

class LmStudioClient;

/**
 * Coordinates one local-model conversation and applies Workspace Auto only to
 * validated, project-local operations. Sensitive or uncertain operations remain
 * explicit, reviewable approval requests.
 */
class AiAgentController final : public QObject {
    Q_OBJECT
public:
    explicit AiAgentController(QObject* parent = nullptr);
    ~AiAgentController() override;

    void setProjectRoot(const QString& projectRoot);
    void setModifiedOpenFiles(const QStringList& filePaths);
    [[nodiscard]] QString projectRoot() const;
    void setActiveEditorContext(const QString& filePath, const QString& text, const QString& selection,
                                bool modified = false);
    void setSelectedModel(const QString& modelId);
    [[nodiscard]] QString selectedModel() const;
    [[nodiscard]] AiAgentState state() const;
    [[nodiscard]] LmStudioClient* client() const;
    [[nodiscard]] QVector<AiChatMessage> messages() const;

    void refreshModels();
    void submitPrompt(const QString& prompt, bool includeActiveFile, bool includeSelection);
    void stop();
    void clearConversation();
    void approvePendingAction(const QString& approvalId);
    void rejectPendingAction(const QString& approvalId);
    void acceptPatchReview(const QString& reviewId);
    void rejectPatchReview(const QString& reviewId);
    void shutdown();

signals:
    void stateChanged(AiAgentState state);
    void modelsAvailable(QVector<AiModelDescriptor> models);
    void messageAdded(AiChatMessage message);
    void assistantTextUpdated(QString text);
    void approvalRequested(AiToolApprovalRequest request);
    void approvalResolved(QString approvalId, bool approved);
    void patchReviewRequested(AiPatchReviewRequest request);
    void patchReviewResolved(QString reviewId, bool accepted);
    void activityAdded(AiActivityEntry entry);
    void toolResultReady(QString toolCallId, QString result, bool success);
    void workspaceFileMutated(QString absolutePath);
    void connectionError(AiTransportError error);

private:
    enum class ToolDisposition : quint8 { AutoExecute, RequireApproval, Reject };

    void setState(AiAgentState state);
    void appendMessage(AiChatRole role, const QString& content, const QString& toolCallId = {}, const QString& name = {});
    void appendAssistantToolCallMessage();
    void appendActivity(AiActivityKind kind, const QString& title, const QString& details = {});
    void finalizeToolCalls();
    void stageToolApproval(const AiToolCall& call, const QString& reason = {});
    void stagePatchReview(const AiToolCall& call, bool automatic);
    void presentNextPatchReview();
    void resolvePatchReview(const QString& reviewId, bool accepted);
    void executeApprovedTool(const AiToolApprovalRequest& request);
    void executeAutomaticTool(const AiToolCall& call);
    void completeToolExecution(const AiToolCall& call, const QString& result, bool success, bool automatic);
    void continueAutonomousTask();
    void beginModelTurn();
    [[nodiscard]] ToolDisposition dispositionFor(const AiToolCall& call, QString* reason) const;
    [[nodiscard]] bool commandRequiresApproval(const QString& command, QString* reason) const;
    [[nodiscard]] QString toolSummary(const AiToolCall& call, const QString& result, bool success) const;
    void executeTerminalCommand(const AiToolApprovalRequest& request, bool automatic = false);
    [[nodiscard]] QString executeReadOnlyTool(const AiToolCall& call, bool* success) const;
    [[nodiscard]] QString executeFileMutation(const AiToolCall& call, bool* success) const;
    [[nodiscard]] QString canonicalProjectPath(const QString& relativePath) const;
    [[nodiscard]] QJsonArray toolDefinitions() const;
    [[nodiscard]] QString systemPrompt() const;

    LmStudioClient* m_client = nullptr;
    QString m_projectRoot;
    QString m_activeFilePath;
    QString m_activeFileText;
    QString m_activeSelection;
    bool m_activeFileModified = false;
    QStringList m_modifiedOpenFiles;
    QString m_selectedModel;
    AiAgentState m_state = AiAgentState::Idle;
    QVector<AiChatMessage> m_messages;
    QHash<QString, QString> m_toolNames;
    QHash<QString, QString> m_toolArguments;
    QHash<QString, AiToolApprovalRequest> m_pendingApprovals;
    QHash<QString, AiPatchReviewRequest> m_pendingPatchReviews;
    QStringList m_patchReviewOrder;
    QString m_visiblePatchReviewId;
    QString m_currentAssistantText;
    QPointer<QProcess> m_commandProcess;
    QTimer m_commandTimeout;
    bool m_workspaceAutoTaskActive = false;
    int m_autonomousStepCount = 0;
    int m_automaticOperationsInFlight = 0;
    bool m_finalizingToolCalls = false;
    QHash<QString, int> m_toolSignatureCounts;
    static constexpr int kMaximumAutonomousSteps = 12;
    static constexpr int kMaximumRepeatedToolSignature = 3;
};
