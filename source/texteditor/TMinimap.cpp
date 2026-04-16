#include "TMinimap.h"
#include "TEditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QApplication>
#include <QScreen>
#include <algorithm> // std::clamp

TMinimap::TMinimap(TEditor *editor, QWidget *parent)
    : QWidget(parent), editor(editor)
{
    setMouseTracking(true);

    // Setup preview label using modern object initialization
    previewLabel = new QLabel(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
    previewLabel->setStyleSheet(
        "QLabel {"
        "  background-color: #1e202e;"
        "  color: #cccccc;"
        "  border: 1px solid #007acc;"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "  font-family: 'Noto Kufi Arabic', 'Courier New', monospace;"
        "  font-size: 10px;"
        "}"
        );
    previewLabel->setTextFormat(Qt::PlainText);
    previewLabel->setMinimumWidth(300);
    previewLabel->setWordWrap(false);
    previewLabel->hide();

    previewTimer = new QTimer(this);
    previewTimer->setSingleShot(true);
    previewTimer->setInterval(5); // Small delay to prevent tooltip spamming

    connect(previewTimer, &QTimer::timeout, this, [this]() {
        if (isHovering && !isDragging) {
            showPreviewTooltip(mapFromGlobal(QCursor::pos()));
        }
    });
}

void TMinimap::updateMinimap() {
    update();
}

void TMinimap::computeLayout(int& outVisibleBlockCount, double& outYRatio,
                             double& outSliderHeight, double& outSliderY) const
{
    outVisibleBlockCount = 0;

    // Fast counting of visible blocks
    for (QTextBlock b = editor->document()->firstBlock(); b.isValid(); b = b.next()) {
        if (b.isVisible()) outVisibleBlockCount++;
    }

    if (outVisibleBlockCount == 0) {
        outYRatio = 1.0;
        outSliderHeight = 0.0;
        outSliderY = 0.0;
        return;
    }

    const double theoreticalHeight = outVisibleBlockCount * MINIMAP_LINE_HEIGHT;
    outYRatio = theoreticalHeight > height() ? static_cast<double>(height()) / theoreticalHeight : 1.0;

    int linesInEditor = 1;
    if (editor->fontMetrics().lineSpacing() > 0) {
        linesInEditor = editor->viewport()->height() / editor->fontMetrics().lineSpacing();
    }

    outSliderHeight = std::max(10.0, linesInEditor * MINIMAP_LINE_HEIGHT * outYRatio);

    const double contentHeight = theoreticalHeight * outYRatio;
    const double maxSliderY = std::max(0.0, contentHeight - outSliderHeight);

    const int maxScroll = editor->verticalScrollBar()->maximum();
    const int currentScroll = editor->verticalScrollBar()->value();

    outSliderY = (maxScroll > 0) ? (static_cast<double>(currentScroll) / maxScroll) * maxSliderY : 0.0;
}

void TMinimap::scrollTo(int y) {
    QScrollBar* vBar = editor->verticalScrollBar();
    if (vBar->maximum() <= 0) return;

    int visibleBlockCount;
    double yRatio, sliderHeight, sliderY;
    computeLayout(visibleBlockCount, yRatio, sliderHeight, sliderY);

    if (visibleBlockCount == 0) return;

    double targetY = static_cast<double>(y) - clickOffset;

    const double contentHeight = visibleBlockCount * MINIMAP_LINE_HEIGHT * yRatio;
    const double maxSliderY = std::max(1.0, contentHeight - sliderHeight);

    targetY = std::clamp(targetY, 0.0, maxSliderY);

    const double ratio = targetY / maxSliderY;
    vBar->setValue(static_cast<int>(ratio * vBar->maximum()));
}

void TMinimap::enterEvent(QEnterEvent* event) {
    setCursor(Qt::PointingHandCursor);
}

void TMinimap::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);
        hidePreviewTooltip();

        int visibleBlockCount;
        double yRatio, sliderHeight, sliderY;
        computeLayout(visibleBlockCount, yRatio, sliderHeight, sliderY);

        const double mouseY = event->pos().y();

        if (mouseY >= sliderY && mouseY <= sliderY + sliderHeight) {
            clickOffset = mouseY - sliderY;
        } else {
            clickOffset = sliderHeight / 2.0;
            scrollTo(mouseY);
        }

        isDragging = true;
    }
}

void TMinimap::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging) {
        setCursor(Qt::ClosedHandCursor);
        scrollTo(event->pos().y());
    } else {
        isHovering = true;
        previewTimer->start();
    }
}

void TMinimap::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
    }
    setCursor(Qt::PointingHandCursor);
}

void TMinimap::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    isHovering = false;
    previewTimer->stop();
    hidePreviewTooltip();
    unsetCursor();
}

void TMinimap::wheelEvent(QWheelEvent* event) {
    // Pass wheel scroll events to the editor so user can scroll while hovering over minimap
    QApplication::sendEvent(editor->verticalScrollBar(), event);
}

void TMinimap::showPreviewTooltip(const QPoint& pos) {
    if (pos.y() < 0 || pos.y() >= height()) {
        hidePreviewTooltip();
        return;
    }

    int visibleBlockCount;
    double yRatio, sliderHeight, sliderY;
    computeLayout(visibleBlockCount, yRatio, sliderHeight, sliderY);

    if (visibleBlockCount == 0 || yRatio <= 0.0) return;

    const int targetVisibleIndex = static_cast<int>(pos.y() / (MINIMAP_LINE_HEIGHT * yRatio));

    int currentVisibleIndex = 0;
    int targetBlockNumber = -1;

    // Fast forward to target block
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

    const int CONTEXT_LINES = 2;
    const int startBlock = std::max(0, targetBlockNumber - CONTEXT_LINES);
    const int endBlock = std::min(editor->document()->blockCount() - 1, targetBlockNumber + CONTEXT_LINES);

    QString previewText;
    previewText.reserve(500); // Pre-allocate memory to prevent reallocation overhead

    for (int i = startBlock; i <= endBlock; ++i) {
        QTextBlock block = editor->document()->findBlockByNumber(i);
        if (!block.isValid()) continue;

        QString line = block.text();
        if (line.length() > 50) {
            line.truncate(45);
            line.append("...");
        }

        previewText += QString("%1 %2\n").arg(i + 1, 4).arg(line);
    }
    previewText.chop(1); // Remove trailing newline efficiently

    previewLabel->setText(previewText);
    previewLabel->adjustSize();

    QPoint globalPos = mapToGlobal(QPoint(width() + 5, pos.y() - previewLabel->height() / 2));
    const QRect screenRect = QGuiApplication::screenAt(globalPos)
                                ? QGuiApplication::screenAt(globalPos)->availableGeometry()
                                : QGuiApplication::primaryScreen()->availableGeometry();

    if (globalPos.x() + previewLabel->width() > screenRect.right()) {
        globalPos.setX(mapToGlobal(QPoint(0, 0)).x() - previewLabel->width() - 5);
    }

    globalPos.setY(std::clamp(globalPos.y(), screenRect.top(), screenRect.bottom() - previewLabel->height()));

    previewLabel->move(globalPos);
    previewLabel->show();
}

void TMinimap::hidePreviewTooltip() {
    if (previewLabel && previewLabel->isVisible()) {
        previewLabel->hide();
    }
}

void TMinimap::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(event->rect(), QColor("#141520"));

    if (editor->document()->blockCount() == 0) return;

    int visibleBlockCount;
    double yRatio, sliderHeight, sliderY;
    computeLayout(visibleBlockCount, yRatio, sliderHeight, sliderY);

    if (visibleBlockCount == 0) return;

    double currentY = 0.0;
    const int widgetHeight = height();
    const double charWidth = 1.2;
    const QColor defaultColor("#cccccc");

    const double fixedIdent = 4.0;

    // Setting Qt::NoPen removes stroke computation overhead for drawing small rects
    painter.setPen(Qt::NoPen);

    QTextBlock block = editor->document()->firstBlock();

    while(block.isValid()) {
        // Early Exit. If we've drawn past the widget bottom, stop processing completely.
        if (currentY > widgetHeight) break;

        if (block.isVisible()) {
            const QString text = block.text();
            const int textLen = text.length();

            // Skip fast if line is empty
            if (textLen > 0) {
                int leadingSpaces = 0;
                while (leadingSpaces < textLen && text.at(leadingSpaces).isSpace()) {
                    leadingSpaces++;
                }

                if (leadingSpaces < textLen) {
                    double indentOffset = leadingSpaces * fixedIdent;
                    double currentX = (width() - 4.0) - indentOffset;

                    QVector<QTextLayout::FormatRange> formats = block.layout()->formats();
                    std::sort(formats.begin(), formats.end(), [](const QTextLayout::FormatRange& a, const QTextLayout::FormatRange& b) {
                        return a.start < b.start;
                    });

                    int stringIdx = leadingSpaces;
                    for (const auto& f : formats) {
                        if (currentX <= 4.0 || stringIdx >= textLen) break;
                        if (f.start + f.length <= stringIdx) continue;

                        if (f.start > stringIdx) {
                            const int gapLen = f.start - stringIdx;
                            double rectW = gapLen * charWidth;
                            rectW = (currentX - rectW < 4.0) ? (currentX - 4.0) : rectW;

                            if (rectW > 0) {
                                painter.fillRect(QRectF(currentX - rectW, currentY, rectW, DOT_HEIGHT), defaultColor);
                                currentX -= rectW;
                            }
                            stringIdx = f.start;
                        }

                        int formatDrawLen = f.length - std::max(0, stringIdx - f.start);
                        if (formatDrawLen > 0) {
                            QColor color = f.format.foreground().color();
                            if (!color.isValid()) color = defaultColor;

                            double rectW = formatDrawLen * charWidth;
                            rectW = (currentX - rectW < 4.0) ? (currentX - 4.0) : rectW;

                            if (rectW > 0) {
                                painter.fillRect(QRectF(currentX - rectW, currentY, rectW, DOT_HEIGHT), color);
                                currentX -= rectW;
                            }
                            stringIdx += formatDrawLen;
                        }
                    }

                    if (currentX > 4.0 && stringIdx < textLen) {
                        const int remain = textLen - stringIdx;
                        double rectW = remain * charWidth;
                        rectW = (currentX - rectW < 4.0) ? (currentX - 4.0) : rectW;

                        if (rectW > 0) {
                            painter.fillRect(QRectF(currentX - rectW, currentY, rectW, DOT_HEIGHT), defaultColor);
                        }
                    }
                }
            }
            currentY += (MINIMAP_LINE_HEIGHT * yRatio);
        }
        block = block.next();
    }

    // Draw the active viewport slider
    painter.fillRect(QRectF(0, sliderY, width(), sliderHeight), QColor(56, 186, 255, 25));
    painter.setPen(QPen(QColor(56, 186, 255, 75), 1));
    painter.drawRect(QRectF(0, sliderY, width() - 1, sliderHeight));
}