#include "TMinimap.h"
#include "TEditor.h"
#include <QPainterPath>
#include <QTextLayout>
#include <QToolTip>
#include <algorithm>

TMinimap::TMinimap(TEditor *editor, QWidget *parent)
    : QWidget(parent), editor(editor), isDragging(false), clickOffset(0)
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true); // لتفعيل أحداث تحريك الماوس بدون ضغط

    // إعداد نافذة المعاينة
    previewLabel = new QLabel(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
    previewLabel->setStyleSheet(
        "QLabel {"
        "  background-color: #1e202e;"
        "  color: #cccccc;"
        "  border: 1px solid #007acc;"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "  font-family: 'Noto Kufi Arabic', 'Courier New', monospace;"
        "  font-size: 11px;"
        "}"
    );
    previewLabel->setTextFormat(Qt::PlainText);
    previewLabel->setWordWrap(false);
    previewLabel->hide();

    previewTimer = new QTimer(this);
    previewTimer->setSingleShot(true);
    // previewTimer->setInterval(50); // تأخير قبل إظهار المعاينة
    connect(previewTimer, &QTimer::timeout, this, [this]() {
        if (isHovering && !isDragging) {
            showPreviewTooltip(mapFromGlobal(QCursor::pos()));
        }
    });
}

void TMinimap::updateMinimap() {
    update();
}

// ============================================================================
// دالة موحدة لحساب أبعاد الشريحة (Slider) - تُستخدم من جميع الدوال
// تحل مشكلة التأخر بين حركة الماوس والخريطة
// ============================================================================
void TMinimap::computeLayout(int& outVisibleBlockCount, double& outYRatio,
                             double& outSliderHeight, double& outSliderY)
{
    outVisibleBlockCount = 0;
    int firstVisIndex = 0;
    int visibleIndex = 0;
    int firstVis = editor->firstVisibleBlock().blockNumber();

    for (QTextBlock b = editor->document()->firstBlock(); b.isValid(); b = b.next()) {
        if (b.isVisible()) {
            if (b.blockNumber() == firstVis) firstVisIndex = visibleIndex;
            visibleIndex++;
            outVisibleBlockCount++;
        }
    }

    double theoreticalHeight = outVisibleBlockCount * MINIMAP_LINE_HEIGHT;
    outYRatio = 1.0;
    if (theoreticalHeight > height()) {
        outYRatio = (double)height() / theoreticalHeight;
    }

    int linesInEditor = 1;
    if (editor->fontMetrics().lineSpacing() > 0) {
        linesInEditor = editor->viewport()->height() / editor->fontMetrics().lineSpacing();
    }

    outSliderHeight = linesInEditor * MINIMAP_LINE_HEIGHT * outYRatio;
    outSliderHeight = qMax(10.0, outSliderHeight);

    outSliderY = firstVisIndex * MINIMAP_LINE_HEIGHT * outYRatio;
}

void TMinimap::scrollTo(int y) {
    if (editor->verticalScrollBar()->maximum() <= 0) return;

    int visibleBlockCount;
    double yRatio, sliderHeight, sliderY;
    computeLayout(visibleBlockCount, yRatio, sliderHeight, sliderY);

    if (visibleBlockCount == 0) return;

    double targetY = y - clickOffset;
    if (targetY < 0) targetY = 0;

    double maxSliderY = height() - sliderHeight;
    if (maxSliderY <= 0) maxSliderY = 1;
    if (targetY > maxSliderY) targetY = maxSliderY;

    double ratio = targetY / maxSliderY;
    int maxScroll = editor->verticalScrollBar()->maximum();
    editor->verticalScrollBar()->setValue(static_cast<int>(ratio * maxScroll));
}

void TMinimap::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        hidePreviewTooltip();

        int visibleBlockCount;
        double yRatio, sliderHeight, sliderY;
        computeLayout(visibleBlockCount, yRatio, sliderHeight, sliderY);

        double mouseY = event->pos().y();

        // إذا ضغط المستخدم داخل الشريحة، نحفظ الفرق
        if (mouseY >= sliderY && mouseY <= sliderY + sliderHeight) {
            clickOffset = mouseY - sliderY;
        } else {
            // إذا ضغط خارج الشريحة، ننقل المنتصف للنقطة المضغوطة
            clickOffset = sliderHeight / 2.0;
            scrollTo(mouseY);
        }

        isDragging = true;
    }
}

void TMinimap::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging) {
        scrollTo(event->pos().y());
    } else {
        // إظهار المعاينة عند التمرير فوق الخريطة
        isHovering = true;
        previewTimer->start();
    }
}

void TMinimap::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
    }
}

void TMinimap::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    isHovering = false;
    previewTimer->stop();
    hidePreviewTooltip();
}

// ============================================================================
// نافذة المعاينة - تُظهر محتوى الأسطر عند التحويم على الخريطة المصغرة
// مثل محرر Kate
// ============================================================================
void TMinimap::showPreviewTooltip(const QPoint& pos) {
    if (pos.y() < 0 || pos.y() >= height()) {
        hidePreviewTooltip();
        return;
    }

    int visibleBlockCount;
    double yRatio, sliderHeight, sliderY;
    computeLayout(visibleBlockCount, yRatio, sliderHeight, sliderY);

    if (visibleBlockCount == 0 || yRatio <= 0) return;

    // تحديد رقم السطر من موضع الماوس
    double lineAtMouse = pos.y() / (MINIMAP_LINE_HEIGHT * yRatio);
    int targetVisibleIndex = static_cast<int>(lineAtMouse);

    // تحويل الفهرس المرئي إلى رقم البلوك الفعلي
    int currentVisibleIndex = 0;
    int targetBlockNumber = -1;
    for (QTextBlock b = editor->document()->firstBlock(); b.isValid(); b = b.next()) {
        if (b.isVisible()) {
            if (currentVisibleIndex == targetVisibleIndex) {
                targetBlockNumber = b.blockNumber();
                break;
            }
            currentVisibleIndex++;
        }
    }

    if (targetBlockNumber < 0) {
        hidePreviewTooltip();
        return;
    }

    // جمع الأسطر المحيطة (5 أسطر قبل و5 بعد)
    const int CONTEXT_LINES = 5;
    int startBlock = qMax(0, targetBlockNumber - CONTEXT_LINES);
    int endBlock = qMin(editor->document()->blockCount() - 1, targetBlockNumber + CONTEXT_LINES);

    QString previewText;
    for (int i = startBlock; i <= endBlock; ++i) {
        QTextBlock block = editor->document()->findBlockByNumber(i);
        if (!block.isValid()) continue;

        QString line = block.text();
        // تقطيع الأسطر الطويلة
        if (line.length() > 80) {
            line = line.left(77) + "...";
        }

        if (i == targetBlockNumber) {
            previewText += QString("► %1: %2").arg(i + 1).arg(line);
        } else {
            previewText += QString("  %1: %2").arg(i + 1).arg(line);
        }

        if (i < endBlock) previewText += "\n";
    }

    previewLabel->setText(previewText);
    previewLabel->adjustSize();

    // حساب موضع المعاينة (على يمين الخريطة)
    QPoint globalPos = mapToGlobal(QPoint(width() + 5, pos.y() - previewLabel->height() / 2));

    // التأكد من أن النافذة لا تخرج عن حدود الشاشة
    QRect screenRect = screen()->availableGeometry();
    if (globalPos.x() + previewLabel->width() > screenRect.right()) {
        globalPos.setX(mapToGlobal(QPoint(0, 0)).x() - previewLabel->width() - 5);
    }
    if (globalPos.y() < screenRect.top()) {
        globalPos.setY(screenRect.top());
    }
    if (globalPos.y() + previewLabel->height() > screenRect.bottom()) {
        globalPos.setY(screenRect.bottom() - previewLabel->height());
    }

    previewLabel->move(globalPos);
    previewLabel->show();
}

void TMinimap::hidePreviewTooltip() {
    if (previewLabel) {
        previewLabel->hide();
    }
}

// ============================================================================
// رسم الخريطة المصغرة
// ============================================================================
void TMinimap::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.fillRect(event->rect(), QColor("#141520"));

    int blockCount = editor->document()->blockCount();
    if (blockCount == 0) return;

    int visibleBlockCount;
    double yRatio, sliderHeight, sliderY;
    computeLayout(visibleBlockCount, yRatio, sliderHeight, sliderY);

    if (visibleBlockCount == 0) return;

    double currentY = 0;
    QTextBlock block = editor->document()->firstBlock();

    QColor defaultColor("#cccccc");
    double charWidth = 1.5;

    while(block.isValid()) {
        if (block.isVisible()) {
            QString text = block.text();

            int leadingSpaces = 0;
            while (leadingSpaces < text.length() && text[leadingSpaces].isSpace()) {
                leadingSpaces++;
            }

            int textLen = text.length();
            if (leadingSpaces < textLen) {
                // في الواجهات التي تعتمد من اليمين لليسار RTL سنبدأ الرسم من مسافة اليمين
                double currentX = width() - 4;
                int rectHeight = static_cast<int>(qMax(1.0, MINIMAP_LINE_HEIGHT * yRatio));

                QVector<QTextLayout::FormatRange> formats = block.layout()->formats();
                std::sort(formats.begin(), formats.end(), [](const QTextLayout::FormatRange& a, const QTextLayout::FormatRange& b) {
                    return a.start < b.start;
                });

                int stringIdx = leadingSpaces;
                for (const auto& f : formats) {
                    if (currentX <= 4 || stringIdx >= textLen) break;
                    if (f.start + f.length <= stringIdx) continue;

                    if (f.start > stringIdx) {
                        int gapLen = f.start - stringIdx;
                        int rectW = gapLen * charWidth;
                        if (currentX - rectW < 4) rectW = currentX - 4;
                        if (rectW > 0) {
                            painter.fillRect(currentX - rectW, static_cast<int>(currentY), rectW, rectHeight, defaultColor);
                            currentX -= rectW;
                        }
                        stringIdx = f.start;
                    }

                    int formatDrawLen = f.length;
                    if (f.start < stringIdx) {
                        formatDrawLen -= (stringIdx - f.start);
                    }
                    if (formatDrawLen > 0) {
                        QColor color = f.format.foreground().color();
                        if (!color.isValid()) color = defaultColor;

                        int rectW = formatDrawLen * charWidth;
                        if (currentX - rectW < 4) rectW = currentX - 4;
                        if (rectW > 0) {
                            painter.fillRect(currentX - rectW, static_cast<int>(currentY), rectW, rectHeight, color);
                            currentX -= rectW;
                        }
                        stringIdx += formatDrawLen;
                    }
                }

                if (currentX > 4 && stringIdx < textLen) {
                    int remain = textLen - stringIdx;
                    int rectW = remain * charWidth;
                    if (currentX - rectW < 4) rectW = currentX - 4;
                    if (rectW > 0) {
                        painter.fillRect(currentX - rectW, static_cast<int>(currentY), rectW, rectHeight, defaultColor);
                    }
                }
            }
            currentY += (MINIMAP_LINE_HEIGHT * yRatio);
        }
        block = block.next();
    }

    // رسم شريحة العرض (المربع المحدد لمنطقة الرؤية)
    painter.fillRect(0, static_cast<int>(sliderY), width(), static_cast<int>(sliderHeight), QColor(255, 255, 255, 30));
    painter.setPen(QPen(QColor(255, 255, 255, 100), 1));
    painter.drawRect(0, static_cast<int>(sliderY), width() - 1, static_cast<int>(sliderHeight));
}
