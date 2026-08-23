#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

/** Typed values exchanged between the LM Studio client, agent controller, and chat panel. */
enum class AiChatRole : quint8 {
    System,
    User,
    Assistant,
    Tool
};

enum class AiConnectionState : quint8 {
    Offline,
    DiscoveringModels,
    Ready,
    Streaming,
    Cancelling,
    Error
};

enum class AiAutonomyMode : quint8 {
    Manual,
    WorkspaceAuto
};

enum class AiAgentState : quint8 {
    Idle,
    DiscoveringModels,
    StreamingResponse,
    AwaitingApproval,
    ExecutingApprovedTool,
    Cancelling,
    Failed
};

enum class AiToolKind : quint8 {
    ListProjectTree,
    ReadProjectFile,
    SearchWorkspace,
    GetActiveEditorContext,
    ProposeFilePatch,
    ProposeCreateFile,
    ProposeRenamePath,
    ProposeDeletePath,
    ProposeTerminalCommand,
    Unknown
};

enum class AiApprovalKind : quint8 {
    ReadOnlyContext,
    FileMutation,
    TerminalCommand
};

enum class AiActivityKind : quint8 {
    UserPrompt,
    AssistantResponse,
    ToolRequested,
    ToolApproved,
    ToolRejected,
    ToolCompleted,
    AutoExecuted,
    Blocked,
    Error
};

struct AiModelDescriptor final {
    QString id;
    QString displayName;
    QString ownedBy;

    [[nodiscard]] bool isValid() const { return !id.trimmed().isEmpty(); }
};

struct AiChatMessage final {
    AiChatRole role = AiChatRole::User;
    QString content;
    QString toolCallId;
    QString name;
    // For assistant turns that request tools, stores the OpenAI-compatible tool_calls array.
    QJsonArray toolCalls;
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
};

struct AiToolCall final {
    QString id;
    AiToolKind kind = AiToolKind::Unknown;
    QString name;
    QJsonObject arguments;
    QString rawArguments;

    [[nodiscard]] bool isValid() const { return !id.isEmpty() && kind != AiToolKind::Unknown; }
};

struct AiToolApprovalRequest final {
    QString approvalId;
    AiApprovalKind kind = AiApprovalKind::ReadOnlyContext;
    AiToolCall toolCall;
    QString title;
    QString details;
    QString preview;
    QStringList affectedPaths;
    QDateTime createdAt = QDateTime::currentDateTimeUtc();
};

/** A validated existing-file patch held for explicit side-by-side review. */
struct AiPatchReviewRequest final {
    QString reviewId;
    AiToolCall toolCall;
    QString relativePath;
    QString absolutePath;
    QString originalText;
    QString proposedText;
    QString originalSha256;
    bool automatic = false;
    // True only while the model is still emitting an incomplete tool argument.
    // Preview data is never eligible for Accept or filesystem mutation.
    bool isStreamingPreview = false;
    QDateTime createdAt = QDateTime::currentDateTimeUtc();

    [[nodiscard]] bool isValid() const
    {
        return !isStreamingPreview && !reviewId.isEmpty()
            && toolCall.kind == AiToolKind::ProposeFilePatch
            && !absolutePath.isEmpty() && !originalSha256.isEmpty();
    }
};

struct AiActivityEntry final {
    AiActivityKind kind = AiActivityKind::UserPrompt;
    QString title;
    QString details;
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
};

struct AiTransportError final {
    QString message;
    int httpStatus = 0;
    bool retryable = false;
};

inline QString aiToolKindName(const AiToolKind kind)
{
    switch (kind) {
    case AiToolKind::ListProjectTree: return QStringLiteral("list_project_tree");
    case AiToolKind::ReadProjectFile: return QStringLiteral("read_project_file");
    case AiToolKind::SearchWorkspace: return QStringLiteral("search_workspace");
    case AiToolKind::GetActiveEditorContext: return QStringLiteral("get_active_editor_context");
    case AiToolKind::ProposeFilePatch: return QStringLiteral("propose_file_patch");
    case AiToolKind::ProposeCreateFile: return QStringLiteral("propose_create_file");
    case AiToolKind::ProposeRenamePath: return QStringLiteral("propose_rename_path");
    case AiToolKind::ProposeDeletePath: return QStringLiteral("propose_delete_path");
    case AiToolKind::ProposeTerminalCommand: return QStringLiteral("propose_terminal_command");
    case AiToolKind::Unknown: break;
    }
    return QStringLiteral("unknown");
}

inline AiToolKind aiToolKindFromName(const QString& name)
{
    if (name == QStringLiteral("list_project_tree")) return AiToolKind::ListProjectTree;
    if (name == QStringLiteral("read_project_file")) return AiToolKind::ReadProjectFile;
    if (name == QStringLiteral("search_workspace")) return AiToolKind::SearchWorkspace;
    if (name == QStringLiteral("get_active_editor_context")) return AiToolKind::GetActiveEditorContext;
    if (name == QStringLiteral("propose_file_patch")) return AiToolKind::ProposeFilePatch;
    if (name == QStringLiteral("propose_create_file")) return AiToolKind::ProposeCreateFile;
    if (name == QStringLiteral("propose_rename_path")) return AiToolKind::ProposeRenamePath;
    if (name == QStringLiteral("propose_delete_path")) return AiToolKind::ProposeDeletePath;
    if (name == QStringLiteral("propose_terminal_command")) return AiToolKind::ProposeTerminalCommand;
    return AiToolKind::Unknown;
}

Q_DECLARE_METATYPE(AiModelDescriptor)
Q_DECLARE_METATYPE(AiChatMessage)
Q_DECLARE_METATYPE(AiToolCall)
Q_DECLARE_METATYPE(AiToolApprovalRequest)
Q_DECLARE_METATYPE(AiPatchReviewRequest)
Q_DECLARE_METATYPE(AiActivityEntry)
Q_DECLARE_METATYPE(AiTransportError)
