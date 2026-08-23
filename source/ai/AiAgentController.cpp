#include "AiAgentController.h"

#include "LmStudioClient.h"
#include "ProjectFileOperations.h"
#include "AiWorkspacePolicy.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>
#include <QRegularExpression>

#include <algorithm>

namespace {
constexpr qsizetype kMaximumTreeEntries = 500;
constexpr qsizetype kMaximumSearchMatches = 200;
constexpr qsizetype kMaximumFileBytes = 512 * 1024;

bool isMutationTool(const AiToolKind kind)
{
    return kind == AiToolKind::ProposeFilePatch || kind == AiToolKind::ProposeCreateFile
        || kind == AiToolKind::ProposeRenamePath || kind == AiToolKind::ProposeDeletePath;
}

AiApprovalKind approvalKindFor(const AiToolKind kind)
{
    if (kind == AiToolKind::ProposeTerminalCommand) return AiApprovalKind::TerminalCommand;
    if (isMutationTool(kind)) return AiApprovalKind::FileMutation;
    return AiApprovalKind::ReadOnlyContext;
}

QString bounded(QString value, const qsizetype maximum)
{
    if (value.size() <= maximum) return value;
    return value.left(maximum) + QStringLiteral("\n… تم اقتطاع الناتج للحد الآمن.");
}
}

AiAgentController::AiAgentController(QObject* const parent)
    : QObject(parent)
    , m_client(new LmStudioClient(this))
{
    m_commandTimeout.setSingleShot(true);
    connect(&m_commandTimeout, &QTimer::timeout, this, [this]() {
        if (m_commandProcess != nullptr && m_commandProcess->state() != QProcess::NotRunning
            && !m_commandProcess->property("taifCommandCompleted").toBool()) {
            m_commandProcess->setProperty("taifCommandTimedOut", true);
            appendActivity(AiActivityKind::Blocked, QStringLiteral("انتهت مهلة أمر الوكيل"),
                           QStringLiteral("تم إيقاف الأمر بعد تجاوز الحد الزمني."));
            m_commandProcess->kill();
        }
    });

    connect(m_client, &LmStudioClient::modelsReceived, this, [this](const QVector<AiModelDescriptor>& models) {
        if (m_selectedModel.isEmpty() && !models.isEmpty()) {
            m_selectedModel = models.first().id;
        }
        setState(AiAgentState::Idle);
        emit modelsAvailable(models);
    });
    connect(m_client, &LmStudioClient::streamDelta, this, [this](const QString& text) {
        m_currentAssistantText.append(text);
        emit assistantTextUpdated(m_currentAssistantText);
    });
    connect(m_client, &LmStudioClient::toolCallDelta, this,
            [this](const QString& id, const QString& name, const QString& argumentsFragment) {
        if (!name.isEmpty()) m_toolNames.insert(id, name);
        m_toolArguments[id].append(argumentsFragment);
        updateStreamingPatchPreview(id);
    });
    connect(m_client, &LmStudioClient::streamFinished, this, [this](const QString& reason) {
        const bool hasToolCalls = !m_toolArguments.isEmpty();
        if (hasToolCalls) {
            appendAssistantToolCallMessage();
        } else if (!m_currentAssistantText.trimmed().isEmpty()) {
            appendMessage(AiChatRole::Assistant, m_currentAssistantText);
            appendActivity(AiActivityKind::AssistantResponse, QStringLiteral("اكتملت استجابة النموذج"));
        }
        m_finalizingToolCalls = true;
        finalizeToolCalls();
        m_finalizingToolCalls = false;
        m_currentAssistantText.clear();
        const QList<AiPatchReviewRequest> unfinishedPreviews = m_streamingPatchPreviews.values();
        m_streamingPatchPreviews.clear();
        for (const AiPatchReviewRequest& preview : unfinishedPreviews) {
            emit patchReviewResolved(preview.reviewId, false);
        }
        m_toolNames.clear();
        m_toolArguments.clear();
        if (reason == QStringLiteral("cancelled")) {
            m_workspaceAutoTaskActive = false;
            setState(AiAgentState::Idle);
            return;
        }
        if (!hasToolCalls) {
            m_workspaceAutoTaskActive = false;
            setState(AiAgentState::Idle);
        } else if (!m_pendingApprovals.isEmpty() || !m_pendingPatchReviews.isEmpty()) {
            setState(AiAgentState::AwaitingApproval);
        } else if (m_automaticOperationsInFlight > 0) {
            setState(AiAgentState::ExecutingApprovedTool);
        } else {
            continueAutonomousTask();
        }
    });
    connect(m_client, &LmStudioClient::requestFailed, this, [this](const AiTransportError& error) {
        m_workspaceAutoTaskActive = false;
        appendActivity(AiActivityKind::Error, QStringLiteral("تعذر الاتصال بالنموذج"), error.message);
        setState(AiAgentState::Failed);
        emit connectionError(error);
    });
}

AiAgentController::~AiAgentController()
{
    shutdown();
}

void AiAgentController::setProjectRoot(const QString& projectRoot)
{
    m_projectRoot = ProjectFileOperations::normalizedPath(projectRoot);
}

void AiAgentController::setModifiedOpenFiles(const QStringList& filePaths)
{
    m_modifiedOpenFiles.clear();
    for (const QString& filePath : filePaths) {
        const QString normalized = ProjectFileOperations::normalizedPath(filePath);
        if (!normalized.isEmpty()) {
            m_modifiedOpenFiles.append(normalized);
        }
    }
    m_modifiedOpenFiles.removeDuplicates();
}

QString AiAgentController::projectRoot() const { return m_projectRoot; }

void AiAgentController::setActiveEditorContext(const QString& filePath, const QString& text,
                                                const QString& selection, const bool modified)
{
    m_activeFilePath = filePath;
    m_activeFileText = text;
    m_activeSelection = selection;
    m_activeFileModified = modified;
}

void AiAgentController::setSelectedModel(const QString& modelId)
{
    m_selectedModel = modelId.trimmed();
}

QString AiAgentController::selectedModel() const { return m_selectedModel; }
AiAgentState AiAgentController::state() const { return m_state; }
LmStudioClient* AiAgentController::client() const { return m_client; }
QVector<AiChatMessage> AiAgentController::messages() const { return m_messages; }

void AiAgentController::refreshModels()
{
    setState(AiAgentState::DiscoveringModels);
    m_client->discoverModels();
}

void AiAgentController::submitPrompt(const QString& prompt, const bool includeActiveFile, const bool includeSelection)
{
    if (prompt.trimmed().isEmpty() || m_client->hasActiveRequest()) return;
    if (m_selectedModel.isEmpty()) {
        emit connectionError({QStringLiteral("لا يوجد نموذج محدد. حدّث قائمة نماذج LM Studio واختر نموذجاً."), 0, true});
        return;
    }

    QString content = prompt.trimmed();
    if (includeSelection && !m_activeSelection.isEmpty()) {
        content += QStringLiteral("\n\n[التحديد النشط من %1]\n%2\n[نهاية التحديد]")
            .arg(QFileInfo(m_activeFilePath).fileName(), bounded(m_activeSelection, m_client->settings().maximumFileContextCharacters));
    } else if (includeActiveFile && !m_activeFileText.isEmpty()) {
        content += QStringLiteral("\n\n[الملف النشط: %1]\n%2\n[نهاية الملف]")
            .arg(QFileInfo(m_activeFilePath).fileName(), bounded(m_activeFileText, m_client->settings().maximumFileContextCharacters));
    }

    if (m_messages.isEmpty()) appendMessage(AiChatRole::System, systemPrompt());
    appendMessage(AiChatRole::User, content);
    appendActivity(AiActivityKind::UserPrompt, QStringLiteral("تم إرسال طلب إلى النموذج"));
    m_workspaceAutoTaskActive = m_client->settings().autonomyMode == AiAutonomyMode::WorkspaceAuto;
    m_autonomousStepCount = 0;
    m_automaticOperationsInFlight = 0;
    m_toolSignatureCounts.clear();
    beginModelTurn();
}

void AiAgentController::stop()
{
    m_workspaceAutoTaskActive = false;
    if (!m_streamingPatchPreviews.isEmpty()) {
        const QList<AiPatchReviewRequest> previews = m_streamingPatchPreviews.values();
        m_streamingPatchPreviews.clear();
        for (const AiPatchReviewRequest& preview : previews) {
            emit patchReviewResolved(preview.reviewId, false);
        }
    }
    if (!m_pendingPatchReviews.isEmpty()) {
        const QStringList reviewIds = m_patchReviewOrder;
        m_pendingPatchReviews.clear();
        m_patchReviewOrder.clear();
        m_visiblePatchReviewId.clear();
        appendActivity(AiActivityKind::Blocked, QStringLiteral("أُلغيت مراجعات تعديلات الوكيل"),
                       QStringLiteral("لم يُكتب أي تعديل معلق إلى الملف."));
        for (const QString& reviewId : reviewIds) {
            emit patchReviewResolved(reviewId, false);
        }
        setState(AiAgentState::Idle);
        return;
    }
    if (m_commandProcess != nullptr && m_commandProcess->state() != QProcess::NotRunning) {
        m_commandTimeout.stop();
        m_commandProcess->kill();
        return;
    }
    if (m_client->hasActiveRequest()) {
        setState(AiAgentState::Cancelling);
        m_client->cancelActiveRequest();
    }
}

void AiAgentController::clearConversation()
{
    stop();
    m_messages.clear();
    m_pendingApprovals.clear();
    m_pendingPatchReviews.clear();
    m_patchReviewOrder.clear();
    m_visiblePatchReviewId.clear();
    m_streamingPatchPreviews.clear();
    m_currentAssistantText.clear();
    m_toolNames.clear();
    m_toolArguments.clear();
    m_workspaceAutoTaskActive = false;
    m_autonomousStepCount = 0;
    m_automaticOperationsInFlight = 0;
    m_toolSignatureCounts.clear();
    setState(AiAgentState::Idle);
}

void AiAgentController::approvePendingAction(const QString& approvalId)
{
    const auto iterator = m_pendingApprovals.find(approvalId);
    if (iterator == m_pendingApprovals.end()) return;
    const AiToolApprovalRequest request = iterator.value();
    m_pendingApprovals.erase(iterator);
    appendActivity(AiActivityKind::ToolApproved, QStringLiteral("تمت الموافقة على إجراء الوكيل"), request.title);
    emit approvalResolved(approvalId, true);
    setState(AiAgentState::ExecutingApprovedTool);
    executeApprovedTool(request);
}

void AiAgentController::rejectPendingAction(const QString& approvalId)
{
    const auto iterator = m_pendingApprovals.find(approvalId);
    if (iterator == m_pendingApprovals.end()) return;
    const AiToolApprovalRequest request = iterator.value();
    m_pendingApprovals.erase(iterator);
    appendActivity(AiActivityKind::ToolRejected, QStringLiteral("تم رفض إجراء الوكيل"), request.title);
    appendMessage(AiChatRole::Tool, QStringLiteral("رفض المستخدم تنفيذ هذا الإجراء."), request.toolCall.id, request.toolCall.name);
    emit approvalResolved(approvalId, false);
    if (m_pendingApprovals.isEmpty()) {
        setState(AiAgentState::Idle);
        continueAutonomousTask();
    }
}

void AiAgentController::shutdown()
{
    m_workspaceAutoTaskActive = false;
    m_commandTimeout.stop();
    m_client->cancelActiveRequest();
    if (m_commandProcess != nullptr) {
        disconnect(m_commandProcess, nullptr, this, nullptr);
        if (m_commandProcess->state() != QProcess::NotRunning) m_commandProcess->kill();
        m_commandProcess->deleteLater();
        m_commandProcess = nullptr;
    }
}

void AiAgentController::setState(const AiAgentState stateValue)
{
    if (m_state == stateValue) return;
    m_state = stateValue;
    emit stateChanged(m_state);
}

void AiAgentController::appendMessage(const AiChatRole role, const QString& content,
                                      const QString& toolCallId, const QString& name)
{
    AiChatMessage message;
    message.role = role;
    message.content = content;
    message.toolCallId = toolCallId;
    message.name = name;
    m_messages.append(message);
    emit messageAdded(message);
}

void AiAgentController::appendAssistantToolCallMessage()
{
    AiChatMessage message;
    message.role = AiChatRole::Assistant;
    message.content = m_currentAssistantText;
    QStringList identifiers = m_toolArguments.keys();
    std::sort(identifiers.begin(), identifiers.end());
    for (const QString& identifier : identifiers) {
        QJsonObject function;
        function.insert(QStringLiteral("name"), m_toolNames.value(identifier));
        const QString arguments = m_toolArguments.value(identifier).trimmed();
        function.insert(QStringLiteral("arguments"), arguments.isEmpty() ? QStringLiteral("{}") : arguments);
        QJsonObject toolCall;
        toolCall.insert(QStringLiteral("id"), identifier);
        toolCall.insert(QStringLiteral("type"), QStringLiteral("function"));
        toolCall.insert(QStringLiteral("function"), function);
        message.toolCalls.append(toolCall);
    }
    if (message.toolCalls.isEmpty()) {
        return;
    }
    m_messages.append(message);
    if (!message.content.trimmed().isEmpty()) {
        emit messageAdded(message);
    }
}

void AiAgentController::appendActivity(const AiActivityKind kind, const QString& title, const QString& details)
{
    emit activityAdded({kind, title, details});
}

void AiAgentController::finalizeToolCalls()
{
    QStringList toolCallIds = m_toolArguments.keys();
    std::sort(toolCallIds.begin(), toolCallIds.end());
    for (const QString& toolCallId : toolCallIds) {
        AiToolCall call;
        call.id = toolCallId;
        call.name = m_toolNames.value(call.id);
        call.kind = aiToolKindFromName(call.name);
        call.rawArguments = m_toolArguments.value(call.id);
        const QJsonDocument arguments = QJsonDocument::fromJson(call.rawArguments.toUtf8());
        if (arguments.isObject()) call.arguments = arguments.object();
        if (!call.isValid() || !arguments.isObject()) {
            appendActivity(AiActivityKind::Error, QStringLiteral("استدعاء أداة غير صالح"), call.name);
            appendMessage(AiChatRole::Tool, QStringLiteral("تعذر تحليل استدعاء الأداة بأمان."), call.id, call.name);
            continue;
        }
        if (call.kind == AiToolKind::ProposeFilePatch) {
            stagePatchReview(call, m_workspaceAutoTaskActive);
            continue;
        }
        QString reason;
        switch (dispositionFor(call, &reason)) {
        case ToolDisposition::AutoExecute:
            executeAutomaticTool(call);
            break;
        case ToolDisposition::RequireApproval:
            stageToolApproval(call, reason);
            break;
        case ToolDisposition::Reject:
            appendActivity(AiActivityKind::Blocked, QStringLiteral("تم حظر إجراء غير آمن"), reason);
            appendMessage(AiChatRole::Tool, QStringLiteral("تم رفض الإجراء: %1").arg(reason), call.id, call.name);
            break;
        }
    }
}

void AiAgentController::stageToolApproval(const AiToolCall& call, const QString& reason)
{
    AiToolCall stagedCall = call;
    if (call.kind == AiToolKind::ProposeFilePatch) {
        const QString absolutePath = canonicalProjectPath(call.arguments.value(QStringLiteral("path")).toString());
        QFile source(absolutePath);
        if (absolutePath.isEmpty() || !source.open(QIODevice::ReadOnly) || source.size() > kMaximumFileBytes) {
            appendActivity(AiActivityKind::Error, QStringLiteral("تم رفض تعديل ملف غير صالح"), call.name);
            appendMessage(AiChatRole::Tool, QStringLiteral("تعذر إعداد تعديل آمن للملف المطلوب."), call.id, call.name);
            return;
        }
        stagedCall.arguments.insert(QStringLiteral("_taif_snapshot_sha256"),
                                    QString::fromLatin1(QCryptographicHash::hash(source.readAll(), QCryptographicHash::Sha256).toHex()));
    }
    AiToolApprovalRequest request;
    request.approvalId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.kind = approvalKindFor(stagedCall.kind);
    request.toolCall = stagedCall;
    request.title = QStringLiteral("تأكيد مطلوب: %1").arg(stagedCall.name);
    request.details = reason.isEmpty() ? QStringLiteral("يتطلب هذا الإجراء مراجعة صريحة.") : reason;
    const QString path = stagedCall.arguments.value(QStringLiteral("path")).toString();
    if (!path.isEmpty()) request.affectedPaths.append(path);
    if (stagedCall.kind == AiToolKind::ProposeTerminalCommand) {
        request.preview = QStringLiteral("سيشغل الوكيل أمراً محلياً من جذر المشروع. يمكن فتح التفاصيل لمراجعته.");
    } else if (stagedCall.kind == AiToolKind::ProposeFilePatch) {
        request.preview = QStringLiteral("سيطبّق الوكيل تعديلاً ذرياً ومحميّاً من التعارض على الملف المحدد.");
    } else if (stagedCall.kind == AiToolKind::ProposeCreateFile) {
        request.preview = QStringLiteral("سينشئ الوكيل ملفاً جديداً فقط إذا لم يكن موجوداً بالفعل.");
    } else {
        request.preview = QStringLiteral("سيتم تنفيذ العملية للقراءة فقط بعد الموافقة.");
    }
    m_pendingApprovals.insert(request.approvalId, request);
    appendActivity(AiActivityKind::ToolRequested, QStringLiteral("يتطلب الإجراء تأكيداً"), stagedCall.name);
    emit approvalRequested(request);
}

void AiAgentController::updateStreamingPatchPreview(const QString& toolCallId)
{
    if (m_toolNames.value(toolCallId) != QStringLiteral("propose_file_patch")) {
        return;
    }
    const QString rawArguments = m_toolArguments.value(toolCallId);
    const auto extractString = [&rawArguments](const QString& key, bool* const complete) -> QString {
        *complete = false;
        const int keyPosition = rawArguments.indexOf(QStringLiteral("\"%1\"").arg(key));
        if (keyPosition < 0) return {};
        const int colon = rawArguments.indexOf(QLatin1Char(':'), keyPosition + key.size() + 2);
        if (colon < 0) return {};
        int position = colon + 1;
        while (position < rawArguments.size() && rawArguments.at(position).isSpace()) ++position;
        if (position >= rawArguments.size() || rawArguments.at(position) != QLatin1Char('\"')) return {};
        ++position;
        QString value;
        bool escaped = false;
        for (; position < rawArguments.size(); ++position) {
            const QChar character = rawArguments.at(position);
            if (escaped) {
                switch (character.unicode()) {
                case 'n': value.append(QLatin1Char('\n')); break;
                case 'r': value.append(QLatin1Char('\r')); break;
                case 't': value.append(QLatin1Char('\t')); break;
                default: value.append(character); break;
                }
                escaped = false;
                continue;
            }
            if (character == QLatin1Char('\\')) {
                escaped = true;
            } else if (character == QLatin1Char('\"')) {
                *complete = true;
                break;
            } else {
                value.append(character);
            }
        }
        return value;
    };

    bool pathComplete = false;
    const QString relativePath = extractString(QStringLiteral("path"), &pathComplete);
    if (!pathComplete || relativePath.trimmed().isEmpty()) {
        return;
    }
    const QString absolutePath = canonicalProjectPath(relativePath);
    QFile source(absolutePath);
    if (absolutePath.isEmpty() || !source.open(QIODevice::ReadOnly) || source.size() > kMaximumFileBytes) {
        return;
    }
    const QByteArray originalBytes = source.read(kMaximumFileBytes + 1);
    if (originalBytes.size() > kMaximumFileBytes || originalBytes.contains('\0')) {
        return;
    }
    bool contentComplete = false;
    const QString proposedText = extractString(QStringLiteral("content"), &contentComplete);
    AiPatchReviewRequest preview;
    preview.reviewId = QStringLiteral("stream-%1").arg(toolCallId);
    preview.toolCall.id = toolCallId;
    preview.toolCall.kind = AiToolKind::ProposeFilePatch;
    preview.toolCall.name = QStringLiteral("propose_file_patch");
    preview.toolCall.arguments.insert(QStringLiteral("path"), relativePath);
    preview.relativePath = QDir(m_projectRoot).relativeFilePath(absolutePath);
    preview.absolutePath = absolutePath;
    preview.originalText = QString::fromUtf8(originalBytes);
    preview.proposedText = proposedText;
    preview.originalSha256 = QString::fromLatin1(QCryptographicHash::hash(originalBytes, QCryptographicHash::Sha256).toHex());
    preview.automatic = m_workspaceAutoTaskActive;
    preview.isStreamingPreview = true;
    m_streamingPatchPreviews.insert(toolCallId, preview);
    emit patchReviewPreviewUpdated(preview);
}

void AiAgentController::stagePatchReview(const AiToolCall& call, const bool automatic)
{
    m_streamingPatchPreviews.remove(call.id);
    const QString signature = aiToolKindName(call.kind) + QLatin1Char(':')
        + QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact));
    if (m_toolSignatureCounts.value(signature) >= kMaximumRepeatedToolSignature) {
        appendActivity(AiActivityKind::Blocked, QStringLiteral("تم إيقاف تعديل متكرر"),
                       QStringLiteral("تكرر اقتراح التعديل نفسه عدة مرات ويحتاج تدخلاً جديداً."));
        appendMessage(AiChatRole::Tool, QStringLiteral("تم إيقاف اقتراح تعديل متكرر لحماية المشروع."), call.id, call.name);
        return;
    }
    ++m_toolSignatureCounts[signature];
    const QString relativePath = call.arguments.value(QStringLiteral("path")).toString();
    const QString absolutePath = canonicalProjectPath(relativePath);
    const QString proposedText = call.arguments.value(QStringLiteral("content")).toString();
    QFile source(absolutePath);
    if (absolutePath.isEmpty() || relativePath.trimmed().isEmpty() || !source.open(QIODevice::ReadOnly)
        || source.size() > kMaximumFileBytes) {
        appendActivity(AiActivityKind::Error, QStringLiteral("تعذر إعداد مراجعة تعديل الملف"), relativePath);
        appendMessage(AiChatRole::Tool, QStringLiteral("تعذر إعداد مراجعة آمنة للملف المطلوب."), call.id, call.name);
        return;
    }
    const QByteArray originalBytes = source.read(kMaximumFileBytes + 1);
    if (originalBytes.size() > kMaximumFileBytes || originalBytes.contains('\0')) {
        appendActivity(AiActivityKind::Blocked, QStringLiteral("تم رفض مراجعة ملف غير نصي"), relativePath);
        appendMessage(AiChatRole::Tool, QStringLiteral("تم رفض تعديل ملف ثنائي أو يتجاوز الحد الآمن."), call.id, call.name);
        return;
    }

    AiPatchReviewRequest review;
    review.reviewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    review.toolCall = call;
    review.toolCall.arguments.insert(QStringLiteral("_taif_snapshot_sha256"),
                                     QString::fromLatin1(QCryptographicHash::hash(originalBytes, QCryptographicHash::Sha256).toHex()));
    review.relativePath = QDir(m_projectRoot).relativeFilePath(absolutePath);
    review.absolutePath = absolutePath;
    review.originalText = QString::fromUtf8(originalBytes);
    review.proposedText = proposedText;
    review.originalSha256 = review.toolCall.arguments.value(QStringLiteral("_taif_snapshot_sha256")).toString();
    review.automatic = automatic;
    m_pendingPatchReviews.insert(review.reviewId, review);
    m_patchReviewOrder.append(review.reviewId);
    appendActivity(AiActivityKind::ToolRequested, QStringLiteral("فتح مراجعة تعديل الوكيل"), review.relativePath);
    presentNextPatchReview();
}

void AiAgentController::presentNextPatchReview()
{
    if (!m_visiblePatchReviewId.isEmpty() && m_pendingPatchReviews.contains(m_visiblePatchReviewId)) {
        return;
    }
    m_visiblePatchReviewId.clear();
    while (!m_patchReviewOrder.isEmpty() && !m_pendingPatchReviews.contains(m_patchReviewOrder.first())) {
        m_patchReviewOrder.removeFirst();
    }
    if (m_patchReviewOrder.isEmpty()) {
        return;
    }
    m_visiblePatchReviewId = m_patchReviewOrder.first();
    setState(AiAgentState::AwaitingApproval);
    emit patchReviewRequested(m_pendingPatchReviews.value(m_visiblePatchReviewId));
}

void AiAgentController::acceptPatchReview(const QString& reviewId)
{
    resolvePatchReview(reviewId, true);
}

void AiAgentController::rejectPatchReview(const QString& reviewId)
{
    resolvePatchReview(reviewId, false);
}

void AiAgentController::resolvePatchReview(const QString& reviewId, const bool accepted)
{
    const auto iterator = m_pendingPatchReviews.find(reviewId);
    if (iterator == m_pendingPatchReviews.end()) {
        return;
    }
    const AiPatchReviewRequest review = iterator.value();
    m_pendingPatchReviews.erase(iterator);
    m_patchReviewOrder.removeAll(reviewId);
    if (m_visiblePatchReviewId == reviewId) {
        m_visiblePatchReviewId.clear();
    }

    if (accepted) {
        appendActivity(AiActivityKind::ToolApproved, QStringLiteral("تم قبول تعديل الوكيل"), review.relativePath);
        setState(AiAgentState::ExecutingApprovedTool);
        bool success = false;
        const QString result = executeFileMutation(review.toolCall, &success);
        completeToolExecution(review.toolCall, result, success, false);
    } else {
        appendActivity(AiActivityKind::ToolRejected, QStringLiteral("تم رفض تعديل الوكيل"), review.relativePath);
        appendMessage(AiChatRole::Tool, QStringLiteral("رفض المستخدم تطبيق تعديل الملف بعد مراجعته."),
                      review.toolCall.id, review.toolCall.name);
        emit toolResultReady(review.toolCall.id, QStringLiteral("رفض المستخدم تطبيق تعديل الملف بعد مراجعته."), false);
    }
    emit patchReviewResolved(reviewId, accepted);

    if (!m_pendingPatchReviews.isEmpty()) {
        presentNextPatchReview();
        return;
    }
    if (review.automatic) {
        continueAutonomousTask();
    } else {
        setState(m_pendingApprovals.isEmpty() ? AiAgentState::Idle : AiAgentState::AwaitingApproval);
    }
}

void AiAgentController::executeApprovedTool(const AiToolApprovalRequest& request)
{
    if (request.toolCall.kind == AiToolKind::ProposeTerminalCommand) {
        executeTerminalCommand(request, false);
        return;
    }
    bool success = false;
    const QString result = isMutationTool(request.toolCall.kind)
        ? executeFileMutation(request.toolCall, &success)
        : executeReadOnlyTool(request.toolCall, &success);
    completeToolExecution(request.toolCall, result, success, false);
    if (m_pendingApprovals.isEmpty()) {
        setState(AiAgentState::Idle);
        continueAutonomousTask();
    } else {
        setState(AiAgentState::AwaitingApproval);
    }
}

void AiAgentController::executeAutomaticTool(const AiToolCall& call)
{
    const QString signature = aiToolKindName(call.kind) + QLatin1Char(':')
        + QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact));
    ++m_toolSignatureCounts[signature];
    ++m_automaticOperationsInFlight;
    appendActivity(AiActivityKind::AutoExecuted, QStringLiteral("ينفذ الوكيل تلقائياً"), call.name);
    if (call.kind == AiToolKind::ProposeTerminalCommand) {
        if (m_commandProcess != nullptr && m_commandProcess->state() != QProcess::NotRunning) {
            completeToolExecution(call, QStringLiteral("يوجد أمر وكيل نشط بالفعل؛ أُجلت المتابعة للمراجعة."), false, true);
            return;
        }
        AiToolApprovalRequest request;
        request.toolCall = call;
        executeTerminalCommand(request, true);
        return;
    }
    AiToolCall executableCall = call;
    if (call.kind == AiToolKind::ProposeFilePatch) {
        const QString absolutePath = canonicalProjectPath(call.arguments.value(QStringLiteral("path")).toString());
        QFile source(absolutePath);
        if (absolutePath.isEmpty() || !source.open(QIODevice::ReadOnly) || source.size() > kMaximumFileBytes) {
            completeToolExecution(call, QStringLiteral("تعذر التحقق من الملف قبل التعديل التلقائي."), false, true);
            return;
        }
        executableCall.arguments.insert(QStringLiteral("_taif_snapshot_sha256"),
                                        QString::fromLatin1(QCryptographicHash::hash(source.readAll(), QCryptographicHash::Sha256).toHex()));
    }
    bool success = false;
    const QString result = isMutationTool(executableCall.kind)
        ? executeFileMutation(executableCall, &success)
        : executeReadOnlyTool(executableCall, &success);
    completeToolExecution(executableCall, result, success, true);
}

void AiAgentController::completeToolExecution(const AiToolCall& call, const QString& result,
                                              const bool success, const bool automatic)
{
    appendMessage(AiChatRole::Tool, result, call.id, call.name);
    appendActivity(success ? AiActivityKind::ToolCompleted : AiActivityKind::Error,
                   success ? QStringLiteral("اكتمل إجراء الوكيل") : QStringLiteral("فشل إجراء الوكيل"),
                   toolSummary(call, result, success));
    emit toolResultReady(call.id, result, success);
    if (success && isMutationTool(call.kind)) {
        const QString path = canonicalProjectPath(call.arguments.value(QStringLiteral("path")).toString());
        if (!path.isEmpty()) {
            emit workspaceFileMutated(path);
        }
    }
    if (automatic) {
        m_automaticOperationsInFlight = qMax(0, m_automaticOperationsInFlight - 1);
        continueAutonomousTask();
    }
}

void AiAgentController::beginModelTurn()
{
    m_currentAssistantText.clear();
    setState(AiAgentState::StreamingResponse);
    m_client->streamChat(m_messages, m_selectedModel, toolDefinitions());
}

void AiAgentController::continueAutonomousTask()
{
    if (!m_workspaceAutoTaskActive || m_finalizingToolCalls || m_client->hasActiveRequest()
        || !m_pendingApprovals.isEmpty() || !m_pendingPatchReviews.isEmpty() || m_automaticOperationsInFlight > 0) {
        return;
    }
    if (++m_autonomousStepCount > kMaximumAutonomousSteps) {
        m_workspaceAutoTaskActive = false;
        appendActivity(AiActivityKind::Blocked, QStringLiteral("توقف التنفيذ التلقائي"),
                       QStringLiteral("بلغ الوكيل الحد الأقصى لخطوات المهمة. راجع النتيجة قبل المتابعة."));
        setState(AiAgentState::Idle);
        return;
    }
    appendActivity(AiActivityKind::AutoExecuted, QStringLiteral("يتابع الوكيل المهمة"),
                   QStringLiteral("خطوة %1 من %2").arg(m_autonomousStepCount).arg(kMaximumAutonomousSteps));
    beginModelTurn();
}

AiAgentController::ToolDisposition AiAgentController::dispositionFor(const AiToolCall& call,
                                                                         QString* const reason) const
{
    if (reason != nullptr) reason->clear();
    if (m_client->settings().autonomyMode != AiAutonomyMode::WorkspaceAuto) {
        if (reason != nullptr) *reason = QStringLiteral("وضع التنفيذ اليدوي مفعل.");
        return ToolDisposition::RequireApproval;
    }
    if (call.kind == AiToolKind::Unknown) {
        if (reason != nullptr) *reason = QStringLiteral("الأداة غير معروفة.");
        return ToolDisposition::Reject;
    }
    if (call.kind == AiToolKind::ProposeDeletePath || call.kind == AiToolKind::ProposeRenamePath) {
        if (reason != nullptr) *reason = QStringLiteral("تغييرات المسار أو الحذف تتطلب مراجعة صريحة دائماً.");
        return ToolDisposition::RequireApproval;
    }
    if (call.kind == AiToolKind::ProposeTerminalCommand) {
        QString commandReason;
        if (commandRequiresApproval(call.arguments.value(QStringLiteral("command")).toString(), &commandReason)) {
            if (reason != nullptr) *reason = commandReason;
            return ToolDisposition::RequireApproval;
        }
    }
    const QString path = call.arguments.value(QStringLiteral("path")).toString();
    if (!path.isEmpty() && canonicalProjectPath(path).isEmpty()) {
        if (reason != nullptr) *reason = QStringLiteral("المسار يقع خارج جذر المشروع.");
        return ToolDisposition::Reject;
    }
    if (call.kind == AiToolKind::ProposeFilePatch) {
        const QString targetPath = canonicalProjectPath(path);
        const bool activeConflict = m_activeFileModified
            && ProjectFileOperations::normalizedPath(m_activeFilePath) == targetPath;
        const bool openConflict = m_modifiedOpenFiles.contains(targetPath, Qt::CaseInsensitive);
        if (activeConflict || openConflict) {
            if (reason != nullptr) *reason = QStringLiteral("الملف مفتوح وفيه تعديلات غير محفوظة؛ راجعه قبل تطبيق تعديل الوكيل.");
            return ToolDisposition::RequireApproval;
        }
    }
    const QString signature = aiToolKindName(call.kind) + QLatin1Char(':')
        + QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact));
    if (m_toolSignatureCounts.value(signature) >= kMaximumRepeatedToolSignature) {
        if (reason != nullptr) *reason = QStringLiteral("تكرر الإجراء نفسه عدة مرات ويحتاج مراجعة.");
        return ToolDisposition::RequireApproval;
    }
    return ToolDisposition::AutoExecute;
}

bool AiAgentController::commandRequiresApproval(const QString& command, QString* const reason) const
{
    return AiWorkspacePolicy::commandRequiresApproval(command, reason);
}

QString AiAgentController::toolSummary(const AiToolCall& call, const QString& result, const bool success) const
{
    const QString state = success ? QStringLiteral("تم بنجاح") : QStringLiteral("لم يكتمل");
    const QString path = call.arguments.value(QStringLiteral("path")).toString();
    if (!path.isEmpty()) return QStringLiteral("%1 — %2").arg(state, path);
    if (call.kind == AiToolKind::SearchWorkspace) {
        const int matches = result.isEmpty() ? 0 : result.count(QLatin1Char('\n')) + 1;
        return QStringLiteral("%1 — %2 نتيجة بحث").arg(state).arg(matches);
    }
    if (call.kind == AiToolKind::ProposeTerminalCommand) {
        return QStringLiteral("%1 — أمر محلي في المشروع").arg(state);
    }
    return state;
}

void AiAgentController::executeTerminalCommand(const AiToolApprovalRequest& request, const bool automatic)
{
    const QString command = request.toolCall.arguments.value(QStringLiteral("command")).toString().simplified();
    if (command.isEmpty() || m_projectRoot.isEmpty()) {
        completeToolExecution(request.toolCall, QStringLiteral("تم رفض الأمر: الأمر أو جذر المشروع غير صالح."),
                              false, automatic);
        setState(AiAgentState::Idle);
        return;
    }

    QStringList parts = command.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    const QString program = parts.takeFirst();
    auto* const process = new QProcess(this);
    m_commandProcess = process;
    process->setWorkingDirectory(m_projectRoot);
    process->setProcessChannelMode(QProcess::MergedChannels);
    const auto complete = [this, process, request, automatic](const QString& result, const bool success) {
        if (process->property("taifCommandCompleted").toBool()) {
            return;
        }
        process->setProperty("taifCommandCompleted", true);
        if (m_commandProcess == process) {
            m_commandTimeout.stop();
            m_commandProcess = nullptr;
        }
        completeToolExecution(request.toolCall, result, success, automatic);
        process->deleteLater();
        if (!automatic) {
            setState(m_pendingApprovals.isEmpty() ? AiAgentState::Idle : AiAgentState::AwaitingApproval);
            if (m_pendingApprovals.isEmpty()) {
                continueAutonomousTask();
            }
        }
    };
    connect(process, &QProcess::finished, this, [this, process, complete](const int exitCode, const QProcess::ExitStatus status) {
        const QString output = bounded(QString::fromUtf8(process->readAll()),
                                       m_client->settings().maximumToolOutputCharacters);
        if (process->property("taifCommandTimedOut").toBool()) {
            complete(QStringLiteral("انتهت مهلة الأمر قبل اكتماله.\n%1").arg(output), false);
            return;
        }
        const bool success = status == QProcess::NormalExit && exitCode == 0;
        complete(QStringLiteral("رمز الخروج: %1\n%2").arg(exitCode).arg(output), success);
    });
    connect(process, &QProcess::errorOccurred, this, [process, complete](const QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            complete(QStringLiteral("تعذر تشغيل الأمر: %1").arg(process->errorString()), false);
        }
    });
    m_commandTimeout.start(m_client->settings().commandTimeoutMilliseconds);
    process->start(program, parts);
}

QString AiAgentController::executeReadOnlyTool(const AiToolCall& call, bool* const success) const
{
    *success = false;
    if (m_projectRoot.isEmpty()) return QStringLiteral("لا يوجد مجلد مشروع مفتوح.");
    if (call.kind == AiToolKind::GetActiveEditorContext) {
        *success = !m_activeFileText.isEmpty();
        return *success ? bounded(m_activeFileText, m_client->settings().maximumFileContextCharacters)
                        : QStringLiteral("لا توجد محتويات ملف نشط متاحة.");
    }
    if (call.kind == AiToolKind::ListProjectTree) {
        QStringList entries;
        QDirIterator iterator(m_projectRoot, QDir::NoDotAndDotDot | QDir::AllEntries,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext() && entries.size() < kMaximumTreeEntries) {
            const QString absolute = iterator.next();
            entries.append(QDir(m_projectRoot).relativeFilePath(absolute));
        }
        *success = true;
        return entries.join(QLatin1Char('\n'));
    }
    const QString path = canonicalProjectPath(call.arguments.value(QStringLiteral("path")).toString());
    if (path.isEmpty()) return QStringLiteral("تم رفض مسار خارج جذر المشروع.");
    if (call.kind == AiToolKind::ReadProjectFile) {
        QFileInfo info(path);
        if (!info.isFile() || info.size() > kMaximumFileBytes) return QStringLiteral("الملف غير متاح أو يتجاوز الحد الآمن للقراءة.");
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return QStringLiteral("تعذر قراءة الملف المطلوب.");
        const QByteArray bytes = file.read(kMaximumFileBytes + 1);
        if (bytes.contains('\0')) return QStringLiteral("تم رفض ملف ثنائي.");
        *success = true;
        return bounded(QString::fromUtf8(bytes), m_client->settings().maximumFileContextCharacters);
    }
    if (call.kind == AiToolKind::SearchWorkspace) {
        const QString query = call.arguments.value(QStringLiteral("query")).toString();
        if (query.trimmed().isEmpty()) return QStringLiteral("عبارة البحث فارغة.");
        QStringList matches;
        QDirIterator iterator(m_projectRoot, QDir::Files, QDirIterator::Subdirectories);
        while (iterator.hasNext() && matches.size() < kMaximumSearchMatches) {
            const QString filePath = iterator.next();
            QFileInfo info(filePath);
            if (info.size() > kMaximumFileBytes) continue;
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) continue;
            const QByteArray bytes = file.read(kMaximumFileBytes + 1);
            if (bytes.contains('\0')) continue;
            const QString text = QString::fromUtf8(bytes);
            const QStringList lines = text.split(QLatin1Char('\n'));
            for (int index = 0; index < lines.size() && matches.size() < kMaximumSearchMatches; ++index) {
                if (lines.at(index).contains(query, Qt::CaseInsensitive)) {
                    matches.append(QStringLiteral("%1:%2: %3").arg(QDir(m_projectRoot).relativeFilePath(filePath)).arg(index + 1).arg(lines.at(index).trimmed()));
                }
            }
        }
        *success = true;
        return matches.join(QLatin1Char('\n'));
    }
    return QStringLiteral("أداة القراءة غير مدعومة.");
}

QString AiAgentController::executeFileMutation(const AiToolCall& call, bool* const success) const
{
    *success = false;
    if (m_projectRoot.isEmpty()) return QStringLiteral("لا يوجد مجلد مشروع مفتوح.");
    const QString path = canonicalProjectPath(call.arguments.value(QStringLiteral("path")).toString());
    if (path.isEmpty()) return QStringLiteral("تم رفض مسار خارج جذر المشروع.");
    if (call.kind == AiToolKind::ProposeFilePatch || call.kind == AiToolKind::ProposeCreateFile) {
        const QString content = call.arguments.value(QStringLiteral("content")).toString();
        QFileInfo info(path);
        if (call.kind == AiToolKind::ProposeFilePatch && !info.isFile()) return QStringLiteral("الملف المطلوب لتطبيق التعديل غير موجود.");
        const bool activeConflict = call.kind == AiToolKind::ProposeFilePatch && m_activeFileModified
            && ProjectFileOperations::normalizedPath(m_activeFilePath) == path;
        const bool openConflict = call.kind == AiToolKind::ProposeFilePatch
            && m_modifiedOpenFiles.contains(path, Qt::CaseInsensitive);
        if (activeConflict || openConflict) {
            return QStringLiteral("لا يمكن تطبيق التعديل لأن الملف مفتوح وفيه تعديلات غير محفوظة.");
        }
        if (call.kind == AiToolKind::ProposeCreateFile && info.exists()) return QStringLiteral("تم رفض إنشاء الملف لأنه موجود بالفعل.");
        const QString expectedHash = call.arguments.value(QStringLiteral("_taif_snapshot_sha256")).toString();
        if (call.kind == AiToolKind::ProposeFilePatch && expectedHash.isEmpty()) {
            return QStringLiteral("تعذر تطبيق تعديل بلا نسخة تحقق آمنة للملف.");
        }
        if (!expectedHash.isEmpty()) {
            QFile current(path);
            if (!current.open(QIODevice::ReadOnly) || QString::fromLatin1(QCryptographicHash::hash(current.readAll(), QCryptographicHash::Sha256).toHex()) != expectedHash) {
                return QStringLiteral("تغير الملف منذ عرض التعديل. راجع الاقتراح مجدداً قبل التطبيق.");
            }
        }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return QStringLiteral("تعذر فتح الملف للكتابة الآمنة.");
        if (file.write(content.toUtf8()) < 0 || !file.commit()) return QStringLiteral("تعذر حفظ التعديل بشكل ذري.");
        *success = true;
        return QStringLiteral("تم حفظ التعديل بعد الموافقة: %1").arg(QDir(m_projectRoot).relativeFilePath(path));
    }
    if (call.kind == AiToolKind::ProposeRenamePath) {
        QString reason;
        const QString newName = call.arguments.value(QStringLiteral("new_name")).toString();
        if (!ProjectFileOperations::isValidChildName(newName, &reason)) return reason;
        const ProjectFileOperationResult result = ProjectFileOperations::renamePath(m_projectRoot, path, newName);
        *success = result.succeeded;
        return result.userMessage;
    }
    if (call.kind == AiToolKind::ProposeDeletePath) {
        const ProjectFileOperationResult result = ProjectFileOperations::moveToTrash(m_projectRoot, path);
        *success = result.succeeded;
        return result.userMessage;
    }
    return QStringLiteral("أداة التعديل غير مدعومة.");
}

QString AiAgentController::canonicalProjectPath(const QString& relativePath) const
{
    if (relativePath.trimmed().isEmpty() || m_projectRoot.isEmpty()) return {};
    const QString candidate = ProjectFileOperations::normalizedPath(QDir(m_projectRoot).absoluteFilePath(relativePath));
    return ProjectFileOperations::isInsideRoot(m_projectRoot, candidate) ? candidate : QString();
}

QJsonArray AiAgentController::toolDefinitions() const
{
    const auto stringProperty = []() {
        return QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}};
    };
    QJsonArray definitions;
    const auto addTool = [&definitions](const QString& name, const QString& description,
                                        const QJsonObject& properties) {
        QJsonObject parameters;
        parameters.insert(QStringLiteral("type"), QStringLiteral("object"));
        parameters.insert(QStringLiteral("properties"), properties);
        QJsonObject function;
        function.insert(QStringLiteral("name"), name);
        function.insert(QStringLiteral("description"), description);
        function.insert(QStringLiteral("parameters"), parameters);
        QJsonObject tool;
        tool.insert(QStringLiteral("type"), QStringLiteral("function"));
        tool.insert(QStringLiteral("function"), function);
        definitions.append(tool);
    };
    addTool(QStringLiteral("list_project_tree"), QStringLiteral("List the current project files."), {});
    addTool(QStringLiteral("read_project_file"), QStringLiteral("Read one text file under the project root."),
            {{QStringLiteral("path"), stringProperty()}});
    addTool(QStringLiteral("search_workspace"), QStringLiteral("Search textual files under the project root."),
            {{QStringLiteral("query"), stringProperty()}});
    addTool(QStringLiteral("get_active_editor_context"), QStringLiteral("Read user-approved current editor context."), {});
    const QJsonObject fileProperties{{QStringLiteral("path"), stringProperty()},
                                     {QStringLiteral("content"), stringProperty()}};
    addTool(QStringLiteral("propose_file_patch"), QStringLiteral("Replace a contained text file only when the policy permits it."), fileProperties);
    addTool(QStringLiteral("propose_create_file"), QStringLiteral("Create a new contained text file only when the policy permits it."), fileProperties);
    addTool(QStringLiteral("propose_rename_path"), QStringLiteral("Request a reviewed rename of a contained project path."),
            {{QStringLiteral("path"), stringProperty()}, {QStringLiteral("new_name"), stringProperty()}});
    addTool(QStringLiteral("propose_delete_path"), QStringLiteral("Request a reviewed move of a contained project path to trash."),
            {{QStringLiteral("path"), stringProperty()}});
    addTool(QStringLiteral("propose_terminal_command"), QStringLiteral("Run only a safe, policy-validated command from the project root."),
            {{QStringLiteral("command"), stringProperty()}});
    return definitions;
}

QString AiAgentController::systemPrompt() const
{
    return QStringLiteral("You are TaifEditor local coding assistant. Respond helpfully in the user's language. "
        "Use available tools for project work and keep paths relative to the project root. Workspace Auto may complete only "
        "policy-validated low-risk operations; other actions are paused for explicit review. Never attempt shell chains, packages, "
        "network access, credentials, elevation, process control, deletion, rename, or an out-of-root path. Never claim a file "
        "or command changed unless its tool result confirms completion. The project language is Alif, an Arabic RTL programming language.");
}
