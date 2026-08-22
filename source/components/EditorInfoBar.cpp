#include "EditorInfoBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QStyle>
#include <QToolButton>

namespace {

constexpr int kCompactWidth = 780;
constexpr int kDenseWidth = 1040;

QString localizedNumber(const qsizetype value)
{
    return QString::number(value);
}

} // namespace

EditorInfoBar::EditorInfoBar(QWidget* const parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("EditorInfoBar"));
    setAccessibleName(QStringLiteral("شريط معلومات المحرر"));
    setLayoutDirection(Qt::RightToLeft);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 2, 8, 2);
    m_layout->setSpacing(4);

    m_documentSegment = createSegment(QStringLiteral("InfoDocumentSegment"), m_documentLabel);
    m_diagnosticsSegment = new QWidget(this);
    m_diagnosticsSegment->setObjectName(QStringLiteral("InfoDiagnosticsSegment"));
    auto* const diagnosticsLayout = new QHBoxLayout(m_diagnosticsSegment);
    diagnosticsLayout->setContentsMargins(8, 2, 8, 2);
    diagnosticsLayout->setSpacing(0);
    m_diagnosticsButton = new QToolButton(m_diagnosticsSegment);
    m_diagnosticsButton->setObjectName(QStringLiteral("InformationDiagnosticsButton"));
    m_diagnosticsButton->setAccessibleName(QStringLiteral("ملخص المشكلات"));
    m_diagnosticsButton->setCursor(Qt::PointingHandCursor);
    diagnosticsLayout->addWidget(m_diagnosticsButton);
    connect(m_diagnosticsButton, &QToolButton::clicked, this, &EditorInfoBar::diagnosticsActivated);

    m_analysisSegment = createSegment(QStringLiteral("InfoAnalysisSegment"), m_analysisLabel);
    m_recoverySegment = createSegment(QStringLiteral("InfoRecoverySegment"), m_recoveryLabel);
    m_selectionSegment = createSegment(QStringLiteral("InfoSelectionSegment"), m_selectionLabel);
    m_cursorSegment = createSegment(QStringLiteral("InfoCursorSegment"), m_cursorLabel);
    m_formatSegment = createSegment(QStringLiteral("InfoFormatSegment"), m_formatLabel);

    m_layout->addWidget(m_documentSegment);
    m_layout->addWidget(m_diagnosticsSegment);
    m_layout->addWidget(m_analysisSegment);
    m_layout->addWidget(m_recoverySegment);
    m_layout->addWidget(m_selectionSegment);
    m_layout->addWidget(m_cursorSegment);
    m_layout->addWidget(m_formatSegment);
    m_layout->addStretch(1);

    setStyleSheet(QStringLiteral(R"(
        QWidget#EditorInfoBar {
            background-color: #0b1324;
            border-top: 1px solid #334155;
            color: #cbd5e1;
            font-family: "Tajawal", "Noto Kufi Arabic";
            font-size: 12px;
        }
        QWidget#InfoDocumentSegment, QWidget#InfoDiagnosticsSegment,
        QWidget#InfoAnalysisSegment, QWidget#InfoRecoverySegment,
        QWidget#InfoSelectionSegment, QWidget#InfoCursorSegment,
        QWidget#InfoFormatSegment {
            background-color: #111d33;
            border: 1px solid #263a57;
            border-radius: 5px;
        }
        QLabel {
            color: #dbeafe;
            padding: 2px 3px;
        }
        QLabel#InfoDocumentSegmentLabel[modified="true"] {
            color: #fbbf24;
        }
        QLabel#InfoAnalysisSegmentLabel[attention="true"],
        QLabel#InfoRecoverySegmentLabel[attention="true"] {
            color: #fbbf24;
        }
        QToolButton#InformationDiagnosticsButton {
            border: none;
            background: transparent;
            color: #93c5fd;
            padding: 2px 3px;
        }
        QToolButton#InformationDiagnosticsButton:hover {
            background-color: #1d4ed8;
            color: #ffffff;
            border-radius: 4px;
        }
        QToolButton#InformationDiagnosticsButton[hasErrors="true"] {
            color: #fca5a5;
        }
        QToolButton#InformationDiagnosticsButton[hasWarnings="true"] {
            color: #fcd34d;
        }
    )"));

    refreshPresentation();
}

void EditorInfoBar::setSnapshot(const EditorInfoSnapshot& snapshot)
{
    m_snapshot = snapshot;
    refreshPresentation();
}

EditorInfoSnapshot EditorInfoBar::snapshot() const
{
    return m_snapshot;
}

void EditorInfoBar::resizeEvent(QResizeEvent* const event)
{
    QWidget::resizeEvent(event);
    applyResponsiveVisibility();
}

QWidget* EditorInfoBar::createSegment(const QString& objectName, QLabel*& label)
{
    auto* const segment = new QWidget(this);
    segment->setObjectName(objectName);
    auto* const layout = new QHBoxLayout(segment);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(0);
    label = new QLabel(segment);
    label->setObjectName(objectName + QStringLiteral("Label"));
    label->setLayoutDirection(Qt::LeftToRight);
    layout->addWidget(label);
    return segment;
}

void EditorInfoBar::refreshPresentation()
{
    if (!m_snapshot.hasEditor) {
        m_documentLabel->setText(QStringLiteral("لا يوجد ملف نشط"));
        m_documentLabel->setProperty("modified", false);
        m_documentLabel->setToolTip(QString());
        m_diagnosticsButton->setText(QStringLiteral("المشكلات: —"));
        m_diagnosticsButton->setToolTip(QStringLiteral("افتح ملفاً لعرض المشكلات."));
        m_analysisLabel->setText(QStringLiteral("التحليل: —"));
        m_recoveryLabel->setText(QStringLiteral("الحفظ: —"));
        m_selectionLabel->setText(QStringLiteral("المستند: —"));
        m_cursorLabel->setText(QStringLiteral("السطر: —"));
        m_formatLabel->setText(QStringLiteral("UTF-8"));
        applyResponsiveVisibility();
        return;
    }

    const QString documentName = m_snapshot.documentName.isEmpty()
        ? QStringLiteral("بدون عنوان") : m_snapshot.documentName;
    m_documentLabel->setText(QStringLiteral("%1%2")
        .arg(m_snapshot.modified ? QStringLiteral("● ") : QString())
        .arg(documentName));
    m_documentLabel->setProperty("modified", m_snapshot.modified);
    m_documentLabel->style()->unpolish(m_documentLabel);
    m_documentLabel->style()->polish(m_documentLabel);
    m_documentLabel->setToolTip(m_snapshot.documentPath.isEmpty()
        ? QStringLiteral("مستند غير محفوظ") : m_snapshot.documentPath);

    m_diagnosticsButton->setText(QStringLiteral("أخطاء %1 · تحذيرات %2")
        .arg(m_snapshot.errorCount).arg(m_snapshot.warningCount));
    m_diagnosticsButton->setToolTip(QStringLiteral("%1 خطأ، %2 تحذير. انقر لفتح المشكلات.")
        .arg(m_snapshot.errorCount).arg(m_snapshot.warningCount));
    m_diagnosticsButton->setProperty("hasErrors", m_snapshot.errorCount > 0);
    m_diagnosticsButton->setProperty("hasWarnings", m_snapshot.errorCount == 0 && m_snapshot.warningCount > 0);
    m_diagnosticsButton->style()->unpolish(m_diagnosticsButton);
    m_diagnosticsButton->style()->polish(m_diagnosticsButton);

    m_analysisLabel->setText(analysisText(m_snapshot));
    m_analysisLabel->setToolTip(QStringLiteral("التحليل: %1 مللي ثانية، %2 رمز، لقطة %3 حرف")
        .arg(m_snapshot.analysisDurationMilliseconds)
        .arg(localizedNumber(m_snapshot.analysisTokenCount))
        .arg(localizedNumber(m_snapshot.analysisSnapshotCharacters)));
    const bool analysisNeedsAttention = m_snapshot.analysisState == EditorInfoSnapshot::AnalysisState::Pending
        || m_snapshot.analysisState == EditorInfoSnapshot::AnalysisState::LargeDocument;
    m_analysisLabel->setProperty("attention", analysisNeedsAttention);
    m_analysisLabel->style()->unpolish(m_analysisLabel);
    m_analysisLabel->style()->polish(m_analysisLabel);

    m_recoveryLabel->setText(recoveryText(m_snapshot));
    m_recoveryLabel->setToolTip(QStringLiteral("آخر كتابة للاستعادة: %1 مللي ثانية")
        .arg(m_snapshot.recoveryWriteDurationMilliseconds));
    const bool recoveryNeedsAttention = m_snapshot.recoveryState != EditorInfoSnapshot::RecoveryState::Clean;
    m_recoveryLabel->setProperty("attention", recoveryNeedsAttention);
    m_recoveryLabel->style()->unpolish(m_recoveryLabel);
    m_recoveryLabel->style()->polish(m_recoveryLabel);

    if (m_snapshot.selectedCharacters > 0) {
        m_selectionLabel->setText(QStringLiteral("تحديد: %1 حرف · %2 سطر")
            .arg(compactCount(m_snapshot.selectedCharacters))
            .arg(compactCount(m_snapshot.selectedLines)));
        m_selectionLabel->setToolTip(QStringLiteral("تحديد %1 حرف عبر %2 سطر")
            .arg(localizedNumber(m_snapshot.selectedCharacters))
            .arg(localizedNumber(m_snapshot.selectedLines)));
    } else {
        m_selectionLabel->setText(QStringLiteral("%1 سطر · %2 حرف")
            .arg(compactCount(m_snapshot.documentLines))
            .arg(compactCount(m_snapshot.documentCharacters)));
        m_selectionLabel->setToolTip(QStringLiteral("المستند: %1 سطر، %2 حرف")
            .arg(localizedNumber(m_snapshot.documentLines))
            .arg(localizedNumber(m_snapshot.documentCharacters)));
    }

    m_cursorLabel->setText(QStringLiteral("سطر %1 · عمود %2")
        .arg(m_snapshot.line).arg(m_snapshot.column));
    m_cursorLabel->setToolTip(QStringLiteral("موضع المؤشر: السطر %1، العمود %2")
        .arg(m_snapshot.line).arg(m_snapshot.column));

    m_formatLabel->setText(QStringLiteral("%1 · %2 · %3 %4")
        .arg(m_snapshot.encoding, lineEndingText(m_snapshot.lineEnding),
             m_snapshot.usesSpaces ? QStringLiteral("مسافات") : QStringLiteral("Tabs"),
             QString::number(m_snapshot.indentationWidth)));
    m_formatLabel->setToolTip(QStringLiteral("الترميز %1، نهاية السطر %2، الإزاحة %3 بعرض %4")
        .arg(m_snapshot.encoding, lineEndingText(m_snapshot.lineEnding),
             m_snapshot.usesSpaces ? QStringLiteral("مسافات") : QStringLiteral("Tabs"),
             QString::number(m_snapshot.indentationWidth)));

    applyResponsiveVisibility();
}

void EditorInfoBar::applyResponsiveVisibility()
{
    const int availableWidth = width();
    const bool compact = availableWidth > 0 && availableWidth < kCompactWidth;
    const bool dense = availableWidth > 0 && availableWidth < kDenseWidth;

    m_documentSegment->setVisible(!compact);
    m_analysisSegment->setVisible(!dense);
    m_recoverySegment->setVisible(!dense);
    m_selectionSegment->setVisible(!compact);
    m_diagnosticsSegment->setVisible(true);
    m_cursorSegment->setVisible(true);
    m_formatSegment->setVisible(true);
}

QString EditorInfoBar::compactCount(const qsizetype value)
{
    if (value >= 1000000) {
        return QString::number(static_cast<double>(value) / 1000000.0, 'f', 1) + QStringLiteral("M");
    }
    if (value >= 1000) {
        return QString::number(static_cast<double>(value) / 1000.0, 'f', 1) + QStringLiteral("K");
    }
    return localizedNumber(value);
}

QString EditorInfoBar::lineEndingText(const EditorInfoSnapshot::LineEnding lineEnding)
{
    switch (lineEnding) {
    case EditorInfoSnapshot::LineEnding::Lf: return QStringLiteral("LF");
    case EditorInfoSnapshot::LineEnding::Crlf: return QStringLiteral("CRLF");
    case EditorInfoSnapshot::LineEnding::Mixed: return QStringLiteral("Mixed EOL");
    case EditorInfoSnapshot::LineEnding::Unknown: return QStringLiteral("EOL");
    }
    return QStringLiteral("EOL");
}

QString EditorInfoBar::analysisText(const EditorInfoSnapshot& snapshot)
{
    switch (snapshot.analysisState) {
    case EditorInfoSnapshot::AnalysisState::Pending:
        return QStringLiteral("التحليل: جاري");
    case EditorInfoSnapshot::AnalysisState::Ready:
        return QStringLiteral("تحليل %1ms").arg(snapshot.analysisDurationMilliseconds);
    case EditorInfoSnapshot::AnalysisState::LargeDocument:
        return QStringLiteral("تحليل: مستند كبير");
    case EditorInfoSnapshot::AnalysisState::Unavailable:
        return QStringLiteral("التحليل: —");
    }
    return QStringLiteral("التحليل: —");
}

QString EditorInfoBar::recoveryText(const EditorInfoSnapshot& snapshot)
{
    switch (snapshot.recoveryState) {
    case EditorInfoSnapshot::RecoveryState::PendingPersistence:
        return QStringLiteral("الحفظ: جارٍ");
    case EditorInfoSnapshot::RecoveryState::RetryScheduled:
        return QStringLiteral("الحفظ: إعادة محاولة");
    case EditorInfoSnapshot::RecoveryState::Clean:
        return QStringLiteral("الحفظ: محفوظ");
    }
    return QStringLiteral("الحفظ: —");
}
