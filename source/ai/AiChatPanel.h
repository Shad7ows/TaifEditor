#pragma once

#include "AiAgentTypes.h"

#include <QWidget>

class AiAgentController;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QTextBrowser;

/** Dock-hosted RTL panel for streaming local AI chat and reviewable agent actions. */
class AiChatPanel final : public QWidget {
    Q_OBJECT
public:
    explicit AiChatPanel(QWidget* parent = nullptr);
    ~AiChatPanel() override;

    void setProjectRoot(const QString& projectRoot);
    void setModifiedOpenFiles(const QStringList& filePaths);
    void setActiveEditorContext(const QString& filePath, const QString& text, const QString& selection,
                                bool modified = false);
    [[nodiscard]] AiAgentController* controller() const;

signals:
    void requestOpenFile(QString path);
    void requestInsertText(QString text, bool replaceSelection);
    void workspaceFileMutated(QString absolutePath);

private:
    void appendTranscript(const AiChatMessage& message);
    void refreshStreamingTranscript(const QString& text);
    void replaceTranscriptDocument(const QString& html);
    void addApprovalCard(const AiToolApprovalRequest& request);
    void addActivity(const AiActivityEntry& activity);
    void showActivityDetails(const AiActivityEntry& activity);
    void updateConnectionState(AiConnectionState state);
    void updateAgentState(AiAgentState state);
    void refreshContextLabel();
    void submitComposer();
    void loadSettings();
    void saveSettings();

    AiAgentController* m_controller = nullptr;
    QLineEdit* m_endpointEdit = nullptr;
    QComboBox* m_modelCombo = nullptr;
    QComboBox* m_timeoutCombo = nullptr;
    QLabel* m_connectionLabel = nullptr;
    QLabel* m_agentStatusLabel = nullptr;
    QLabel* m_contextLabel = nullptr;
    QTextBrowser* m_transcript = nullptr;
    QPlainTextEdit* m_composer = nullptr;
    QCheckBox* m_includeFile = nullptr;
    QCheckBox* m_includeSelection = nullptr;
    QCheckBox* m_workspaceAuto = nullptr;
    QPushButton* m_sendButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QListWidget* m_approvals = nullptr;
    QListWidget* m_activity = nullptr;
    QString m_activeFilePath;
    QString m_activeText;
    QString m_activeSelection;
    QString m_streamingHtml;
    QString m_transcriptHtml;
};
