#include "AiPatchReviewWidget.h"

#include "AiLineDiff.h"

#include <QAbstractTextDocumentLayout>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBlock>
#include <QTextCursor>
#include <QVBoxLayout>

#include <algorithm>

namespace {
QPlainTextEdit* createSourcePane(const QString& objectName, const QString& placeholder, QWidget* const parent)
{
    auto* const pane = new QPlainTextEdit(parent);
    pane->setObjectName(objectName);
    pane->setReadOnly(true);
    pane->setUndoRedoEnabled(false);
    pane->setLineWrapMode(QPlainTextEdit::NoWrap);
    pane->setLayoutDirection(Qt::LeftToRight);
    pane->setPlaceholderText(placeholder);
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(qMax(9, font.pointSize()));
    pane->setFont(font);
    return pane;
}

QTextEdit::ExtraSelection lineSelection(QPlainTextEdit* const editor, const int line, const QColor& color)
{
    QTextEdit::ExtraSelection selection;
    const QTextBlock block = editor->document()->findBlockByNumber(line - 1);
    if (!block.isValid()) {
        return selection;
    }
    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    selection.cursor = cursor;
    selection.format.setBackground(color);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    return selection;
}
}

AiPatchReviewWidget::AiPatchReviewWidget(QWidget* const parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("AiPatchReviewWorkspace"));
    setLayoutDirection(Qt::RightToLeft);

    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 9, 10, 9);
    layout->setSpacing(7);

    auto* const header = new QHBoxLayout;
    m_title = new QLabel(QStringLiteral("مراجعة تعديل الوكيل"), this);
    m_title->setObjectName(QStringLiteral("AiPatchReviewTitle"));
    m_title->setWordWrap(true);
    m_queueLabel = new QLabel(this);
    m_queueLabel->setObjectName(QStringLiteral("AiPatchReviewQueue"));
    header->addWidget(m_title, 1);
    header->addWidget(m_queueLabel);
    layout->addLayout(header);

    m_summary = new QLabel(QStringLiteral("لا يكتب هذا العرض أي تعديل قبل موافقتك الصريحة."), this);
    m_summary->setObjectName(QStringLiteral("AiPatchReviewSummary"));
    m_summary->setWordWrap(true);
    layout->addWidget(m_summary);

    auto* const captions = new QHBoxLayout;
    auto* const proposedCaption = new QLabel(QStringLiteral("الاقتراح الجديد"), this);
    proposedCaption->setObjectName(QStringLiteral("AiPatchReviewCaption"));
    proposedCaption->setLayoutDirection(Qt::RightToLeft);
    auto* const originalCaption = new QLabel(QStringLiteral("الملف الحالي"), this);
    originalCaption->setObjectName(QStringLiteral("AiPatchReviewCaption"));
    originalCaption->setLayoutDirection(Qt::RightToLeft);
    captions->addWidget(proposedCaption, 1);
    captions->addWidget(originalCaption, 1);
    layout->addLayout(captions);

    auto* const splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("AiPatchReviewSplitter"));
    m_original = createSourcePane(QStringLiteral("AiPatchReviewOriginal"), QStringLiteral("لا يوجد ملف أصلي"), splitter);
    m_proposed = createSourcePane(QStringLiteral("AiPatchReviewProposed"), QStringLiteral("لا يوجد اقتراح"), splitter);
    // RTL UI chrome places the current source on the right and proposed revision on the left.
    splitter->addWidget(m_original);
    splitter->addWidget(m_proposed);
    splitter->setSizes({600, 600});
    layout->addWidget(splitter, 1);

    auto* const actions = new QHBoxLayout;
    m_previousChange = new QPushButton(QStringLiteral("التغيير السابق"), this);
    m_nextChange = new QPushButton(QStringLiteral("التغيير التالي"), this);
    m_reject = new QPushButton(QStringLiteral("رفض التعديل"), this);
    m_reject->setObjectName(QStringLiteral("AiPatchReviewReject"));
    m_accept = new QPushButton(QStringLiteral("قبول التعديل"), this);
    m_accept->setObjectName(QStringLiteral("AiPatchReviewAccept"));
    actions->addWidget(m_previousChange);
    actions->addWidget(m_nextChange);
    actions->addStretch(1);
    actions->addWidget(m_reject);
    actions->addWidget(m_accept);
    layout->addLayout(actions);

    connect(m_original->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](const int) {
        synchronizeScrollBars(m_original->verticalScrollBar(), m_proposed->verticalScrollBar());
    });
    connect(m_proposed->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](const int) {
        synchronizeScrollBars(m_proposed->verticalScrollBar(), m_original->verticalScrollBar());
    });
    connect(m_previousChange, &QPushButton::clicked, this, [this]() { navigateChange(-1); });
    connect(m_nextChange, &QPushButton::clicked, this, [this]() { navigateChange(1); });
    connect(m_accept, &QPushButton::clicked, this, [this]() {
        if (!m_reviewId.isEmpty()) emit acceptRequested(m_reviewId);
    });
    connect(m_reject, &QPushButton::clicked, this, [this]() {
        if (!m_reviewId.isEmpty()) emit rejectRequested(m_reviewId);
    });
    clearReview();
}

void AiPatchReviewWidget::setReview(const AiPatchReviewRequest& review, const int queuePosition, const int queueSize)
{
    m_reviewId = review.reviewId;
    m_title->setText(QStringLiteral("مراجعة تعديل الوكيل: %1").arg(review.relativePath));
    m_queueLabel->setText(queueSize > 1
        ? QStringLiteral("مراجعة %1 من %2").arg(queuePosition).arg(queueSize) : QString());
    m_original->setPlainText(review.originalText);
    m_proposed->setPlainText(review.proposedText);

    const AiLineDiffResult diff = AiLineDiff::compare(review.originalText, review.proposedText);
    m_summary->setText(QStringLiteral("%1 سطر مضاف، %2 سطر محذوف. لن يُحفظ الملف حتى تختار «قبول التعديل».%3")
        .arg(diff.summary.addedLines).arg(diff.summary.removedLines)
        .arg(diff.summary.usedWholeDocumentFallback ? QStringLiteral(" تم تبسيط العرض للملف الكبير.") : QString()));

    QVector<int> originalRows;
    QVector<int> proposedRows;
    m_changeRows.clear();
    for (const AiLineDiffRow& row : diff.rows) {
        if (row.kind == AiLineDiffRow::Kind::Removed && row.originalLine > 0) {
            originalRows.append(row.originalLine);
            m_changeRows.append(row.originalLine);
        } else if (row.kind == AiLineDiffRow::Kind::Added && row.proposedLine > 0) {
            proposedRows.append(row.proposedLine);
            m_changeRows.append(row.proposedLine);
        }
    }
    std::sort(m_changeRows.begin(), m_changeRows.end());
    m_changeRows.erase(std::unique(m_changeRows.begin(), m_changeRows.end()), m_changeRows.end());
    m_currentChange = -1;
    highlightRows(originalRows, proposedRows);
    const bool hasChanges = !m_changeRows.isEmpty();
    m_previousChange->setEnabled(hasChanges);
    m_nextChange->setEnabled(hasChanges);
    m_accept->setEnabled(review.isValid());
    m_reject->setEnabled(review.isValid());
    if (hasChanges) navigateChange(1);
}

void AiPatchReviewWidget::clearReview()
{
    m_reviewId.clear();
    m_changeRows.clear();
    m_currentChange = -1;
    m_title->setText(QStringLiteral("مراجعة تعديل الوكيل"));
    m_queueLabel->clear();
    m_summary->setText(QStringLiteral("لا توجد مراجعة تعديل معلقة."));
    m_original->clear();
    m_proposed->clear();
    m_previousChange->setEnabled(false);
    m_nextChange->setEnabled(false);
    m_accept->setEnabled(false);
    m_reject->setEnabled(false);
}

QString AiPatchReviewWidget::reviewId() const { return m_reviewId; }
QPlainTextEdit* AiPatchReviewWidget::originalPane() const { return m_original; }
QPlainTextEdit* AiPatchReviewWidget::proposedPane() const { return m_proposed; }

void AiPatchReviewWidget::synchronizeScrollBars(QScrollBar* const source, QScrollBar* const target)
{
    if (m_synchronizingScroll || source->maximum() <= 0 || target->maximum() <= 0) return;
    m_synchronizingScroll = true;
    target->setValue(qRound((static_cast<double>(source->value()) / source->maximum()) * target->maximum()));
    m_synchronizingScroll = false;
}

void AiPatchReviewWidget::highlightRows(const QVector<int>& originalRows, const QVector<int>& proposedRows)
{
    QList<QTextEdit::ExtraSelection> originalSelections;
    QList<QTextEdit::ExtraSelection> proposedSelections;
    for (const int line : originalRows) {
        const QTextEdit::ExtraSelection selection = lineSelection(m_original, line, QColor(QStringLiteral("#4a1f2a")));
        if (selection.cursor.hasSelection()) originalSelections.append(selection);
    }
    for (const int line : proposedRows) {
        const QTextEdit::ExtraSelection selection = lineSelection(m_proposed, line, QColor(QStringLiteral("#173b31")));
        if (selection.cursor.hasSelection()) proposedSelections.append(selection);
    }
    m_original->setExtraSelections(originalSelections);
    m_proposed->setExtraSelections(proposedSelections);
}

void AiPatchReviewWidget::navigateChange(const int direction)
{
    if (m_changeRows.isEmpty()) return;
    m_currentChange = (m_currentChange + direction + m_changeRows.size()) % m_changeRows.size();
    const int line = m_changeRows.at(m_currentChange);
    for (QPlainTextEdit* const editor : {m_original, m_proposed}) {
        const QTextBlock block = editor->document()->findBlockByNumber(line - 1);
        if (!block.isValid()) continue;
        QTextCursor cursor(block);
        editor->setTextCursor(cursor);
        editor->centerCursor();
    }
}
