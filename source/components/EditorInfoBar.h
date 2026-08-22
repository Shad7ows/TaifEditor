#pragma once

#include "EditorInfoSnapshot.h"

#include <QWidget>

class QLabel;
class QToolButton;
class QHBoxLayout;

/**
 * Compact RTL presenter for the active editor's immutable information snapshot.
 * Technical values retain LTR directionality inside the Arabic status-bar shell.
 */
class EditorInfoBar final : public QWidget {
    Q_OBJECT
public:
    explicit EditorInfoBar(QWidget* parent = nullptr);

    void setSnapshot(const EditorInfoSnapshot& snapshot);
    [[nodiscard]] EditorInfoSnapshot snapshot() const;

signals:
    void diagnosticsActivated();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    [[nodiscard]] QWidget* createSegment(const QString& objectName, QLabel*& label);
    void refreshPresentation();
    void applyResponsiveVisibility();
    [[nodiscard]] static QString compactCount(qsizetype value);
    [[nodiscard]] static QString lineEndingText(EditorInfoSnapshot::LineEnding lineEnding);
    [[nodiscard]] static QString analysisText(const EditorInfoSnapshot& snapshot);
    [[nodiscard]] static QString recoveryText(const EditorInfoSnapshot& snapshot);

    EditorInfoSnapshot m_snapshot;
    QHBoxLayout* m_layout = nullptr;
    QWidget* m_documentSegment = nullptr;
    QWidget* m_diagnosticsSegment = nullptr;
    QWidget* m_analysisSegment = nullptr;
    QWidget* m_recoverySegment = nullptr;
    QWidget* m_selectionSegment = nullptr;
    QWidget* m_cursorSegment = nullptr;
    QWidget* m_formatSegment = nullptr;
    QLabel* m_documentLabel = nullptr;
    QToolButton* m_diagnosticsButton = nullptr;
    QLabel* m_analysisLabel = nullptr;
    QLabel* m_recoveryLabel = nullptr;
    QLabel* m_selectionLabel = nullptr;
    QLabel* m_cursorLabel = nullptr;
    QLabel* m_formatLabel = nullptr;
};
