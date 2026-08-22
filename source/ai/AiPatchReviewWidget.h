#pragma once

#include "AiAgentTypes.h"

#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QScrollBar;

/**
 * Read-only central workspace for reviewing a staged AI modification before it
 * reaches disk. Arabic controls are RTL; source panes stay LTR by design.
 */
class AiPatchReviewWidget final : public QWidget {
    Q_OBJECT
public:
    explicit AiPatchReviewWidget(QWidget* parent = nullptr);

    void setReview(const AiPatchReviewRequest& review, int queuePosition = 1, int queueSize = 1);
    void clearReview();
    [[nodiscard]] QString reviewId() const;
    [[nodiscard]] QPlainTextEdit* originalPane() const;
    [[nodiscard]] QPlainTextEdit* proposedPane() const;

signals:
    void acceptRequested(QString reviewId);
    void rejectRequested(QString reviewId);

private:
    void synchronizeScrollBars(QScrollBar* source, QScrollBar* target);
    void highlightRows(const QVector<int>& originalRows, const QVector<int>& proposedRows);
    void navigateChange(int direction);

    QString m_reviewId;
    QVector<int> m_changeRows;
    int m_currentChange = -1;
    bool m_synchronizingScroll = false;
    QLabel* m_title = nullptr;
    QLabel* m_summary = nullptr;
    QLabel* m_queueLabel = nullptr;
    QPlainTextEdit* m_original = nullptr;
    QPlainTextEdit* m_proposed = nullptr;
    QPushButton* m_previousChange = nullptr;
    QPushButton* m_nextChange = nullptr;
    QPushButton* m_accept = nullptr;
    QPushButton* m_reject = nullptr;
};
