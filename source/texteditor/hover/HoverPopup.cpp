#include "HoverPopup.h"

#include "AutoCompleteUI.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

namespace {

CompletionSemanticKind completionKindForSymbol(const SymbolKind kind) {
    switch (kind) {
    case SymbolKind::Function: return CompletionSemanticKind::Function;
    case SymbolKind::Class: return CompletionSemanticKind::Class;
    case SymbolKind::Field: return CompletionSemanticKind::Field;
    case SymbolKind::Parameter: return CompletionSemanticKind::Parameter;
    case SymbolKind::LoopVariable:
    case SymbolKind::ComprehensionVariable: return CompletionSemanticKind::LoopVariable;
    case SymbolKind::ImportModule:
    case SymbolKind::ImportMember: return CompletionSemanticKind::Import;
    case SymbolKind::Builtin: return CompletionSemanticKind::Builtin;
    case SymbolKind::Local: return CompletionSemanticKind::Local;
    case SymbolKind::Module:
    case SymbolKind::External:
    case SymbolKind::Error: return CompletionSemanticKind::Unknown;
    }
    return CompletionSemanticKind::Unknown;
}

QString escapedWithBreaks(const QString& text) {
    return text.toHtmlEscaped().replace(QChar(u'\n'), QStringLiteral("<br>"));
}

} // namespace

THoverPopup::THoverPopup(QWidget* parent) : QFrame(parent) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::NoFocus);
    setLayoutDirection(Qt::RightToLeft);
    setFrameShape(QFrame::NoFrame);
    setMinimumWidth(350);
    setMaximumWidth(520);
    setStyleSheet(
        "THoverPopup { background-color: #1e202e; border: 1px solid #4b5263; "
        "border-top: 2px solid #4793FF; border-radius: 10px; }"
        "QLabel { border: none; background: transparent; }");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 9, 10, 9);
    layout->setSpacing(7);

    auto* headerRow = new QWidget(this);
    headerRow->setLayoutDirection(Qt::LeftToRight);
    auto* headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(9);

    // The fixed left zone intentionally mirrors the autocomplete delegate:
    // semantic glyph left, Arabic text in the right/main reading area.
    iconLabel = new QLabel(headerRow);
    iconLabel->setFixedWidth(35);
    iconLabel->setMinimumHeight(38);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFont(QFont(QStringLiteral("Consolas"), 9, QFont::Bold));
    headerLayout->addWidget(iconLabel);

    headerLabel = new QLabel(headerRow);
    headerLabel->setTextFormat(Qt::RichText);
    headerLabel->setWordWrap(true);
    headerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    headerLabel->setLayoutDirection(Qt::RightToLeft);
    headerLayout->addWidget(headerLabel, 1);
    layout->addWidget(headerRow);

    metadataLabel = new QLabel(this);
    metadataLabel->setTextFormat(Qt::RichText);
    metadataLabel->setWordWrap(true);
    metadataLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    metadataLabel->setLayoutDirection(Qt::RightToLeft);
    metadataLabel->setStyleSheet(
        "QLabel { border-top: 1px solid #3e4451; padding-top: 6px; "
        "font-family: 'Tajawal', sans-serif; }");
    layout->addWidget(metadataLabel);

    // This is visually equivalent to the autocomplete documentation footer.
    documentationLabel = new QLabel(this);
    documentationLabel->setTextFormat(Qt::RichText);
    documentationLabel->setWordWrap(true);
    documentationLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    documentationLabel->setLayoutDirection(Qt::RightToLeft);
    documentationLabel->setMaximumWidth(496);
    documentationLabel->setStyleSheet(
        "QLabel { background-color: #2c313a; border-top: 1px solid #4793FF; "
        "border-radius: 6px; padding: 8px; font-family: 'Tajawal', sans-serif; }");
    layout->addWidget(documentationLabel);
}

void THoverPopup::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setRenderHint(QPainter::Antialiasing, true);

    constexpr qreal cornerRadius = 10.0;
    const QRectF panel = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath panelPath;
    panelPath.addRoundedRect(panel, cornerRadius, cornerRadius);

    // Paint explicitly: top-level Qt::ToolTip windows otherwise may receive the
    // platform's default white native surface instead of stylesheet background.
    painter.fillPath(panelPath, QColor(30, 32, 46)); // Autocomplete dark-blue surface.
    painter.setPen(QPen(QColor(75, 82, 99), 1));
    painter.drawPath(panelPath);

    painter.save();
    painter.setClipPath(panelPath);
    painter.setPen(QPen(QColor(71, 147, 255), 2));
    painter.drawLine(QPointF(panel.left() + cornerRadius, panel.top() + 1.0),
                     QPointF(panel.right() - cornerRadius, panel.top() + 1.0));
    painter.restore();
}

void THoverPopup::setHoverInfo(const HoverInfo& info) {
    const CompletionVisual visual = completionVisual(
        CompletionType::SemanticSymbol, completionKindForSymbol(info.symbolKind));
    iconLabel->setText(visual.icon);
    iconLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; background-color: #2c313a; border-left: 2px solid %1; "
        "font-family: Consolas; }").arg(visual.color.name()));
    headerLabel->setText(QStringLiteral(
        "<div dir='rtl'><span style='color:%1; font-family:Tajawal,sans-serif; "
        "font-size:12px; font-weight:bold;'>%2</span><br>"
        "<span style='color:#f1f5f9; font-family:Consolas,Tajawal,sans-serif; "
        "font-size:14px; font-weight:bold;'>%3</span></div>")
        .arg(visual.color.name(), escapedWithBreaks(info.typeLabel),
             escapedWithBreaks(info.signature)));
    metadataLabel->setText(QStringLiteral(
        "<div dir='rtl'><span style='color:#9da5b4;'>النوع:</span> "
        "<span style='color:#dcdfe4;'>%1</span>"
        "&nbsp;&nbsp;&nbsp;<span style='color:#9da5b4;'>التعريف:</span> "
        "<span style='color:#dcdfe4;'>السطر %2</span></div>")
        .arg(escapedWithBreaks(info.typeLabel), QString::number(info.declarationLine)));
    documentationLabel->setText(QStringLiteral(
        "<div dir='rtl'><span style='color:%1; font-weight:bold;'>التوثيق</span><br>"
        "<span style='color:#dcdfe4; font-size:12px;'>%2</span></div>")
        .arg(visual.color.name(), escapedWithBreaks(info.documentation)));
    adjustSize();
}
