#include "AiChatPanel.h"

#include "AiAgentController.h"
#include "AiAssistantSettings.h"
#include "LmStudioClient.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextBrowser>
#include <QTextDocument>
#include <QVBoxLayout>

namespace {
QString htmlEscaped(const QString& text) { return text.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br/>")); }

QString markdownHtml(const QString& markdown)
{
    // Escape any supplied HTML before conversion: Markdown formatting is supported,
    // while model output cannot inject active markup into the transcript.
    QTextDocument document;
    document.setMarkdown(markdown.toHtmlEscaped(), QTextDocument::MarkdownDialectGitHub);
    const QString fullHtml = document.toHtml();
    const qsizetype bodyTag = fullHtml.indexOf(QStringLiteral("<body"));
    if (bodyTag < 0) {
        return htmlEscaped(markdown);
    }
    const qsizetype bodyStart = fullHtml.indexOf(QLatin1Char('>'), bodyTag);
    const qsizetype bodyEnd = fullHtml.lastIndexOf(QStringLiteral("</body>"));
    if (bodyStart < 0 || bodyEnd <= bodyStart) {
        return htmlEscaped(markdown);
    }
    return fullHtml.mid(bodyStart + 1, bodyEnd - bodyStart - 1);
}

QString assistantMarkdownBody(const QString& markdown)
{
    return QStringLiteral("<style>"
                          "p, li, h1, h2, h3, h4, h5, h6, blockquote { direction:rtl; text-align:right; }"
                          "pre { direction:ltr; text-align:left; background:#0b1628; border:1px solid #263a57; padding:7px; }"
                          "code { direction:ltr; }"
                          "a { color:#93c5fd; }"
                          "</style>%1").arg(markdownHtml(markdown));
}

QString roleLabel(const AiChatRole role)
{
    switch (role) {
    case AiChatRole::System: return QStringLiteral("النظام");
    case AiChatRole::User: return QStringLiteral("أنت");
    case AiChatRole::Assistant: return QStringLiteral("المساعد");
    case AiChatRole::Tool: return QStringLiteral("نتيجة الإجراء");
    }
    return QStringLiteral("المساعد");
}
}

AiChatPanel::AiChatPanel(QWidget* const parent)
    : QWidget(parent)
    , m_controller(new AiAgentController())
{
    setObjectName(QStringLiteral("AiChatPanel"));
    setMinimumWidth(330);

    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(7);

    auto* const title = new QLabel(QStringLiteral("مساعد الذكاء الاصطناعي المحلي"), this);
    title->setObjectName(QStringLiteral("AiPanelTitle"));
    QFont titleFont = title->font(); titleFont.setBold(true); titleFont.setPointSize(titleFont.pointSize() + 1); title->setFont(titleFont);
    layout->addWidget(title);

    auto* const connectionLayout = new QHBoxLayout;
    m_connectionLabel = new QLabel(this); m_connectionLabel->setObjectName(QStringLiteral("AiConnectionLabel"));
    m_endpointEdit = new QLineEdit(this); m_endpointEdit->setLayoutDirection(Qt::LeftToRight); m_endpointEdit->setPlaceholderText(QStringLiteral("http://127.0.0.1:1234/v1"));
    auto* const saveEndpointButton = new QPushButton(QStringLiteral("حفظ"), this);
    auto* const refreshModelsButton = new QPushButton(QStringLiteral("تحديث النماذج"), this);
    connectionLayout->addWidget(m_connectionLabel);
    connectionLayout->addWidget(m_endpointEdit, 1);
    connectionLayout->addWidget(saveEndpointButton);
    connectionLayout->addWidget(refreshModelsButton);
    layout->addLayout(connectionLayout);

    auto* const modelLayout = new QHBoxLayout;
    modelLayout->addWidget(new QLabel(QStringLiteral("النموذج:"), this));
    m_modelCombo = new QComboBox(this); m_modelCombo->setLayoutDirection(Qt::LeftToRight);
    modelLayout->addWidget(m_modelCombo, 1);
    layout->addLayout(modelLayout);

    auto* const agentControls = new QHBoxLayout;
    m_workspaceAuto = new QCheckBox(QStringLiteral("تنفيذ تلقائي داخل المشروع"), this);
    m_workspaceAuto->setObjectName(QStringLiteral("AiWorkspaceAuto"));
    m_timeoutCombo = new QComboBox(this);
    m_timeoutCombo->setObjectName(QStringLiteral("AiTimeoutCombo"));
    m_timeoutCombo->setLayoutDirection(Qt::LeftToRight);
    m_timeoutCombo->addItem(QStringLiteral("5 دقائق"), 300000);
    m_timeoutCombo->addItem(QStringLiteral("10 دقائق"), 600000);
    m_timeoutCombo->addItem(QStringLiteral("20 دقيقة"), 1200000);
    m_timeoutCombo->addItem(QStringLiteral("30 دقيقة"), 1800000);
    agentControls->addWidget(m_workspaceAuto, 1);
    agentControls->addWidget(new QLabel(QStringLiteral("مهلة النموذج:"), this));
    agentControls->addWidget(m_timeoutCombo);
    layout->addLayout(agentControls);

    m_agentStatusLabel = new QLabel(this);
    m_agentStatusLabel->setObjectName(QStringLiteral("AiAgentStatusLabel"));
    m_agentStatusLabel->setWordWrap(true);
    layout->addWidget(m_agentStatusLabel);

    m_contextLabel = new QLabel(this); m_contextLabel->setObjectName(QStringLiteral("AiContextLabel"));
    m_contextLabel->setWordWrap(true);
    layout->addWidget(m_contextLabel);

    m_transcript = new QTextBrowser(this);
    m_transcript->setObjectName(QStringLiteral("AiChatTranscript"));
    m_transcript->setReadOnly(true);
    m_transcript->setOpenExternalLinks(false);
    layout->addWidget(m_transcript, 4);

    auto* const contextToggles = new QHBoxLayout;
    m_includeFile = new QCheckBox(QStringLiteral("إرفاق الملف النشط"), this);
    m_includeSelection = new QCheckBox(QStringLiteral("إرفاق التحديد"), this);
    contextToggles->addWidget(m_includeFile);
    contextToggles->addWidget(m_includeSelection);
    contextToggles->addStretch(1);
    layout->addLayout(contextToggles);

    m_composer = new QPlainTextEdit(this);
    // set RTL
    QTextDocument* editorDocument = m_transcript->document();
    QTextOption option = editorDocument->defaultTextOption();
    option.setTextDirection(Qt::RightToLeft);
    option.setAlignment(Qt::AlignRight);
    editorDocument->setDefaultTextOption(option);
    m_composer->setObjectName(QStringLiteral("AiChatComposer"));
    m_composer->setPlaceholderText(QStringLiteral("اكتب طلبك للمساعد المحلي…"));
    m_composer->setMaximumBlockCount(2000);
    m_composer->setFixedHeight(100);
    layout->addWidget(m_composer);

    auto* const actionLayout = new QHBoxLayout;
    auto* const clearButton = new QPushButton(QStringLiteral("مسح المحادثة"), this);
    m_stopButton = new QPushButton(QStringLiteral("إيقاف"), this); m_stopButton->setEnabled(false);
    m_sendButton = new QPushButton(QStringLiteral("إرسال"), this);
    actionLayout->addWidget(clearButton);
    actionLayout->addStretch(1);
    actionLayout->addWidget(m_stopButton);
    actionLayout->addWidget(m_sendButton);
    layout->addLayout(actionLayout);

    auto* const approvalsTitle = new QLabel(QStringLiteral("إجراءات بانتظار الموافقة"), this);
    approvalsTitle->setObjectName(QStringLiteral("AiSectionTitle"));
    layout->addWidget(approvalsTitle);
    m_approvals = new QListWidget(this);
    m_approvals->setObjectName(QStringLiteral("AiApprovalList"));
    m_approvals->setMinimumHeight(132);
    m_approvals->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_approvals->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    layout->addWidget(m_approvals, 2);

    auto* const activityTitle = new QLabel(QStringLiteral("سجل النشاط — انقر مرتين لعرض التفاصيل"), this);
    activityTitle->setObjectName(QStringLiteral("AiSectionTitle"));
    layout->addWidget(activityTitle);
    m_activity = new QListWidget(this);
    m_activity->setObjectName(QStringLiteral("AiActivityList"));
    m_activity->setMaximumHeight(82);
    m_activity->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    layout->addWidget(m_activity);

    connect(saveEndpointButton, &QPushButton::clicked, this, &AiChatPanel::saveSettings);
    connect(refreshModelsButton, &QPushButton::clicked, m_controller, &AiAgentController::refreshModels);
    connect(m_sendButton, &QPushButton::clicked, this, &AiChatPanel::submitComposer);
    connect(m_stopButton, &QPushButton::clicked, m_controller, &AiAgentController::stop);
    connect(clearButton, &QPushButton::clicked, m_controller, &AiAgentController::clearConversation);
    connect(m_modelCombo, &QComboBox::currentTextChanged, m_controller, &AiAgentController::setSelectedModel);
    connect(m_workspaceAuto, &QCheckBox::toggled, this, [this](const bool) {
        saveSettings();
        updateAgentState(m_controller->state());
    });
    connect(m_timeoutCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](const int) {
        saveSettings();
        updateAgentState(m_controller->state());
    });
    connect(m_controller, &AiAgentController::modelsAvailable, this, [this](const QVector<AiModelDescriptor>& models) {
        const QString selected = m_controller->selectedModel();
        m_modelCombo->blockSignals(true); m_modelCombo->clear();
        for (const AiModelDescriptor& model : models) m_modelCombo->addItem(model.displayName, model.id);
        const int index = m_modelCombo->findData(selected);
        m_modelCombo->setCurrentIndex(index >= 0 ? index : 0);
        m_modelCombo->blockSignals(false);
        if (m_modelCombo->currentIndex() >= 0) m_controller->setSelectedModel(m_modelCombo->currentData().toString());
    });
    connect(m_modelCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](const int index) {
        if (index >= 0) m_controller->setSelectedModel(m_modelCombo->itemData(index).toString());
    });
    connect(m_controller->client(), &LmStudioClient::stateChanged, this, &AiChatPanel::updateConnectionState);
    connect(m_controller, &AiAgentController::stateChanged, this, &AiChatPanel::updateAgentState);
    connect(m_controller, &AiAgentController::messageAdded, this, &AiChatPanel::appendTranscript);
    connect(m_controller, &AiAgentController::assistantTextUpdated, this, &AiChatPanel::refreshStreamingTranscript);
    connect(m_controller, &AiAgentController::approvalRequested, this, &AiChatPanel::addApprovalCard);
    connect(m_controller, &AiAgentController::activityAdded, this, &AiChatPanel::addActivity);
    connect(m_controller, &AiAgentController::workspaceFileMutated,
            this, &AiChatPanel::workspaceFileMutated);
    connect(m_controller, &AiAgentController::connectionError, this, [this](const AiTransportError& error) {
        m_connectionLabel->setText(QStringLiteral("خطأ: %1").arg(error.message));
    });
    connect(m_activity, &QListWidget::itemActivated, this, [this](QListWidgetItem* const item) {
        if (item == nullptr) {
            return;
        }
        AiActivityEntry activity;
        activity.title = item->data(Qt::UserRole).toString();
        activity.details = item->data(Qt::UserRole + 1).toString();
        showActivityDetails(activity);
    });

    loadSettings();
    refreshContextLabel();
    updateConnectionState(m_controller->client()->state());
}

AiChatPanel::~AiChatPanel()
{
    if (m_controller != nullptr) {
        m_controller->shutdown();
        delete m_controller;
        m_controller = nullptr;
    }
}

void AiChatPanel::setProjectRoot(const QString& projectRoot) { m_controller->setProjectRoot(projectRoot); refreshContextLabel(); }

void AiChatPanel::setModifiedOpenFiles(const QStringList& filePaths)
{
    m_controller->setModifiedOpenFiles(filePaths);
}

void AiChatPanel::setActiveEditorContext(const QString& filePath, const QString& text,
                                           const QString& selection, const bool modified)
{
    m_activeFilePath = filePath; m_activeText = text; m_activeSelection = selection;
    m_controller->setActiveEditorContext(filePath, text, selection, modified);
    refreshContextLabel();
}

AiAgentController* AiChatPanel::controller() const { return m_controller; }

void AiChatPanel::appendTranscript(const AiChatMessage& message)
{
    if (message.role == AiChatRole::Tool) {
        const QString title = message.name.isEmpty() ? QStringLiteral("اكتملت خطوة للوكيل")
                                                     : QStringLiteral("اكتملت خطوة: %1").arg(message.name);
        m_transcriptHtml += QStringLiteral("<div dir='rtl' style='margin:8px 2px;padding:7px;border:1px solid #31547f;border-radius:6px;color:#cbd5e1;'>"
                                           "<b style='color:#93c5fd'>%1</b><br/>"
                                           "يظهر ملخص التنفيذ في سجل النشاط. تفاصيل الأداة لا تُعرض داخل المحادثة.</div>")
            .arg(htmlEscaped(title));
        replaceTranscriptDocument(m_transcriptHtml);
        return;
    }
    const QString role = roleLabel(message.role);
    const bool assistantResponse = message.role == AiChatRole::Assistant;
    const QString body = assistantResponse ? assistantMarkdownBody(message.content) : htmlEscaped(message.content);
    m_transcriptHtml += QStringLiteral("<div dir='rtl' align='right' style='direction:rtl;text-align:right;margin:8px 2px;padding:7px;border:1px solid #263a57;border-radius:6px;'>"
        "<b style='color:#93c5fd'>%1</b><br/>%2</div>").arg(htmlEscaped(role), body);
    m_streamingHtml.clear();
    replaceTranscriptDocument(m_transcriptHtml);
}

void AiChatPanel::refreshStreamingTranscript(const QString& text)
{
    m_streamingHtml = QStringLiteral("<div dir='rtl' align='right' style='direction:rtl;text-align:right;margin:8px 2px;padding:7px;border:1px solid #31547f;border-radius:6px;'>"
        "<b style='color:#60a5fa'>المساعد</b><br/>%1</div>").arg(assistantMarkdownBody(text));
    replaceTranscriptDocument(m_transcriptHtml + m_streamingHtml);
}

void AiChatPanel::replaceTranscriptDocument(const QString& html)
{
    QScrollBar* const scrollBar = m_transcript->verticalScrollBar();
    const bool followLatest = scrollBar->value() >= scrollBar->maximum() - 2;
    m_transcript->setUpdatesEnabled(false);
    m_transcript->setHtml(html);
    m_transcript->document()->setDefaultTextOption(QTextOption(Qt::AlignRight));
    if (followLatest) {
        QTextCursor cursor = m_transcript->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_transcript->setTextCursor(cursor);
        scrollBar->setValue(scrollBar->maximum());
    }
    m_transcript->setUpdatesEnabled(true);
    if (followLatest) {
        QTimer::singleShot(0, m_transcript, [browser = m_transcript]() {
            if (browser == nullptr) {
                return;
            }
            browser->verticalScrollBar()->setValue(browser->verticalScrollBar()->maximum());
        });
    }
}

void AiChatPanel::addApprovalCard(const AiToolApprovalRequest& request)
{
    auto* const item = new QListWidgetItem(m_approvals);
    auto* const card = new QFrame(m_approvals);
    card->setObjectName(QStringLiteral("AiApprovalCard"));
    card->setFrameShape(QFrame::StyledPanel);
    auto* const layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 7, 8, 7);
    layout->setSpacing(5);

    auto* const title = new QLabel(request.title, card);
    title->setObjectName(QStringLiteral("AiApprovalTitle"));
    title->setWordWrap(true);
    layout->addWidget(title);

    auto* const preview = new QLabel(request.preview, card);
    preview->setObjectName(QStringLiteral("AiApprovalPreview"));
    preview->setWordWrap(true);
    preview->setLayoutDirection(Qt::LeftToRight);
    preview->setMaximumHeight(42);
    preview->setToolTip(request.preview);
    layout->addWidget(preview);

    auto* const buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 1, 0, 0);
    buttons->setSpacing(7);
    auto* const reject = new QPushButton(QStringLiteral("رفض"), card);
    reject->setObjectName(QStringLiteral("AiRejectApprovalButton"));
    auto* const approve = new QPushButton(QStringLiteral("موافقة"), card);
    approve->setObjectName(QStringLiteral("AiApproveApprovalButton"));
    for (QPushButton* const button : {reject, approve}) {
        button->setMinimumHeight(32);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    buttons->addWidget(reject, 1);
    buttons->addWidget(approve, 1);
    layout->addLayout(buttons);

    item->setSizeHint(card->sizeHint());
    m_approvals->setItemWidget(item, card);
    connect(approve, &QPushButton::clicked, this, [this, item, request]() {
        m_controller->approvePendingAction(request.approvalId);
        delete m_approvals->takeItem(m_approvals->row(item));
    });
    connect(reject, &QPushButton::clicked, this, [this, item, request]() {
        m_controller->rejectPendingAction(request.approvalId);
        delete m_approvals->takeItem(m_approvals->row(item));
    });
}

void AiChatPanel::addActivity(const AiActivityEntry& activity)
{
    auto* const item = new QListWidgetItem(QStringLiteral("%1 — %2").arg(activity.title, activity.details), m_activity);
    item->setData(Qt::UserRole, activity.title);
    item->setData(Qt::UserRole + 1, activity.details);
    item->setToolTip(QStringLiteral("انقر مرتين لعرض ملخص التفاصيل."));
    m_activity->scrollToBottom();
}

void AiChatPanel::showActivityDetails(const AiActivityEntry& activity)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("تفاصيل نشاط الوكيل"));
    dialog.setLayoutDirection(Qt::RightToLeft);
    dialog.resize(520, 280);
    auto* const layout = new QVBoxLayout(&dialog);
    auto* const details = new QPlainTextEdit(&dialog);
    details->setReadOnly(true);
    details->setPlainText(QStringLiteral("%1\n\n%2\n\nلا تعرض هذه النافذة محتوى المصدر أو خرج الأدوات الخام.")
                              .arg(activity.title, activity.details));
    layout->addWidget(details);
    auto* const buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}

void AiChatPanel::updateConnectionState(const AiConnectionState state)
{
    const QString label = state == AiConnectionState::Ready ? QStringLiteral("LM Studio جاهز")
        : state == AiConnectionState::Streaming ? QStringLiteral("يتلقى الاستجابة…")
        : state == AiConnectionState::DiscoveringModels ? QStringLiteral("يبحث عن النماذج…")
        : state == AiConnectionState::Cancelling ? QStringLiteral("جارٍ الإيقاف…")
        : state == AiConnectionState::Error ? QStringLiteral("تعذر الاتصال") : QStringLiteral("غير متصل");
    m_connectionLabel->setText(label);
}

void AiChatPanel::updateAgentState(const AiAgentState state)
{
    const bool active = state == AiAgentState::StreamingResponse || state == AiAgentState::ExecutingApprovedTool || state == AiAgentState::Cancelling;
    m_sendButton->setEnabled(!active);
    m_stopButton->setEnabled(active);
    const QString status = state == AiAgentState::StreamingResponse ? QStringLiteral("الوكيل يحلل المهمة ويستقبل استجابة النموذج…")
        : state == AiAgentState::ExecutingApprovedTool ? QStringLiteral("الوكيل ينفذ خطوة داخل المشروع…")
        : state == AiAgentState::AwaitingApproval ? QStringLiteral("توجد خطوة حساسة تنتظر تأكيدك.")
        : state == AiAgentState::Cancelling ? QStringLiteral("جارٍ إيقاف الوكيل…")
        : state == AiAgentState::Failed ? QStringLiteral("توقّف الوكيل بسبب خطأ. راجع سجل النشاط.")
        : m_workspaceAuto->isChecked() ? QStringLiteral("وضع Workspace Auto: ينجز خطوات المشروع الآمنة تلقائياً.")
        : QStringLiteral("الوضع اليدوي: ستتم مراجعة كل خطوة قبل التنفيذ.");
    m_agentStatusLabel->setText(status);
}

void AiChatPanel::refreshContextLabel()
{
    m_contextLabel->setText(m_activeFilePath.isEmpty() ? QStringLiteral("السياق: لا يوجد ملف نشط مرفق.")
        : QStringLiteral("السياق المتاح: %1%2").arg(m_activeFilePath, m_activeSelection.isEmpty() ? QString() : QStringLiteral(" — يوجد تحديد")));
}

void AiChatPanel::submitComposer()
{
    const QString prompt = m_composer->toPlainText();
    if (prompt.trimmed().isEmpty()) return;
    m_controller->submitPrompt(prompt, m_includeFile->isChecked(), m_includeSelection->isChecked());
    m_composer->clear();
}

void AiChatPanel::loadSettings()
{
    const AiAssistantSettings settings = m_controller->client()->settings();
    const QSignalBlocker endpointBlocker(m_endpointEdit);
    const QSignalBlocker autoBlocker(m_workspaceAuto);
    const QSignalBlocker timeoutBlocker(m_timeoutCombo);
    m_endpointEdit->setText(settings.endpointUrl);
    m_workspaceAuto->setChecked(settings.autonomyMode == AiAutonomyMode::WorkspaceAuto);
    const int timeoutIndex = m_timeoutCombo->findData(settings.requestTimeoutMilliseconds);
    m_timeoutCombo->setCurrentIndex(timeoutIndex >= 0 ? timeoutIndex : 1);
    updateAgentState(m_controller->state());
}

void AiChatPanel::saveSettings()
{
    AiAssistantSettings settings = m_controller->client()->settings();
    const QString endpoint = m_endpointEdit->text().trimmed();
    const bool endpointChanged = endpoint != settings.endpointUrl;
    settings.endpointUrl = endpoint;
    settings.requestTimeoutMilliseconds = m_timeoutCombo->currentData().toInt();
    settings.autonomyMode = m_workspaceAuto->isChecked() ? AiAutonomyMode::WorkspaceAuto : AiAutonomyMode::Manual;
    if (AiAssistantSettingsStore::isLoopbackEndpoint(settings.endpointUrl)) {
        settings.remoteEndpointAcknowledged = true;
    } else if (endpointChanged) {
        m_connectionLabel->setText(QStringLiteral("لا يُحفظ خادم بعيد دون إقرار أمني صريح."));
        return;
    }
    QString error;
    if (!AiAssistantSettingsStore::save(settings, &error)) {
        m_connectionLabel->setText(error);
        return;
    }
    m_controller->client()->setSettings(settings);
    m_connectionLabel->setText(QStringLiteral("تم حفظ إعدادات الاتصال"));
}
