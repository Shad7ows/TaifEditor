#include "TerminalView.h"

#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextLayout>
#include <QTextOption>

#include <utility>

TerminalView::TerminalView(QWidget* const parent)
    : QAbstractScrollArea(parent)
    , m_screen(80, 24)
    , m_parser(m_screen)
    , m_resizeDebounce(this)
    , m_updateTimer(this)
{
    setObjectName(QStringLiteral("NativeTerminalView"));
    setAccessibleName(QStringLiteral("عرض الطرفية الأصلية"));
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_terminalFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_terminalFont.setPixelSize(15);
    setFont(m_terminalFont);
    const QFontMetrics metrics(m_terminalFont);
    m_cellSize = QSize(qMax(7, metrics.horizontalAdvance(QLatin1Char('M'))), qMax(14, metrics.height()));

    m_resizeDebounce.setSingleShot(true);
    m_resizeDebounce.setInterval(80);
    connect(&m_resizeDebounce, &QTimer::timeout, this, [this]() {
        emit gridSizeChanged(gridSize());
    });

    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(16);
    connect(&m_updateTimer, &QTimer::timeout, this, [this]() {
        m_updatePending = false;
        viewport()->update();
    });

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](const int value) {
        m_scrollOffset = value;
        scheduleViewportUpdate();
    });

    m_cursorClock.start();
    recalculateGrid();
}

void TerminalView::appendOutput(const QByteArray& bytes)
{
    const QString previousTitle = m_screen.title();
    const bool followTail = verticalScrollBar()->value() == verticalScrollBar()->maximum();
    m_parser.feed(bytes);
    updateScrollBar(followTail);
    if (previousTitle != m_screen.title()) {
        emit terminalTitleChanged(m_screen.title());
    }
    scheduleViewportUpdate();
}

void TerminalView::clearTerminal()
{
    m_screen.clearAll();
    m_selectionAnchor = {};
    m_selectionExtent = {};
    updateScrollBar(true);
    scheduleViewportUpdate();
}

TerminalScreenModel& TerminalView::screen() { return m_screen; }
const TerminalScreenModel& TerminalView::screen() const { return m_screen; }
QSize TerminalView::gridSize() const { return QSize(m_screen.columns(), m_screen.rows()); }

QString TerminalView::selectedText() const
{
    if (!hasSelection()) {
        return {};
    }

    const int columns = m_screen.columns();
    const int begin = qMin(m_selectionAnchor.row * columns + m_selectionAnchor.column,
                           m_selectionExtent.row * columns + m_selectionExtent.column);
    const int end = qMax(m_selectionAnchor.row * columns + m_selectionAnchor.column,
                         m_selectionExtent.row * columns + m_selectionExtent.column);
    const int totalRows = visualRowCount();
    const int lastRow = end / columns;

    QString result;
    result.reserve((end - begin + 1) * 2);

    for (int index = begin; index <= end; ++index) {
        const int row = index / columns;
        const int column = index % columns;
        if (row >= 0 && row < totalRows) {
            const auto& cell = visualRowAt(row).at(column);
            result += cell.text.isEmpty() ? QLatin1Char(' ') : cell.text;
            if (column == columns - 1 && row != lastRow) {
                result += QLatin1Char('\n');
            }
        }
    }
    return result;
}

bool TerminalView::viewportEvent(QEvent* const event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent*>(event));
        return true;
    default:
        return QAbstractScrollArea::viewportEvent(event);
    }
}

void TerminalView::paintEvent(QPaintEvent* const event)
{
    Q_UNUSED(event);

    QPainter painter(viewport());
    const QColor bgDefault = defaultBackground();
    const QColor fgDefault = defaultForeground();

    painter.fillRect(viewport()->rect(), bgDefault);
    painter.setFont(m_terminalFont);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const int columns = m_screen.columns();
    const int cellWidth = m_cellSize.width();
    const int cellHeight = m_cellSize.height();
    const int totalRows = visualRowCount();
    const int visibleRows = visibleRowCount();
    const int firstRow = qBound(0, m_scrollOffset, qMax(0, totalRows - visibleRows));
    const int lastRow = qMin(totalRows, firstRow + visibleRows);

    const bool hasSel = hasSelection();
    const int selectionStart = hasSel
                                   ? qMin(m_selectionAnchor.row * columns + m_selectionAnchor.column,
                                          m_selectionExtent.row * columns + m_selectionExtent.column)
                                   : -1;
    const int selectionEnd = hasSel
                                 ? qMax(m_selectionAnchor.row * columns + m_selectionAnchor.column,
                                        m_selectionExtent.row * columns + m_selectionExtent.column)
                                 : -1;

    const bool cursorVisible = m_hasFocus && m_screen.cursor().visible
                               && (m_cursorClock.elapsed() / 500) % 2 == 0
                               && m_scrollOffset == verticalScrollBar()->maximum();

    const int cursorVisualRow = cursorVisible
                                    ? static_cast<int>(m_screen.scrollback().size()) + m_screen.cursor().row
                                    : -1;
    const int cursorColumn = cursorVisible ? m_screen.cursor().column : -1;

    QTextOption rowOption;
    rowOption.setWrapMode(QTextOption::NoWrap);
    rowOption.setTextDirection(Qt::RightToLeft);
    rowOption.setAlignment(Qt::AlignRight);
    rowOption.setUseDesignMetrics(true);

    const QColor selectionColor(QStringLiteral("#294b78"));

    for (int row = firstRow; row < lastRow; ++row) {
        const auto& cells = visualRowAt(row);
        const qreal rowY = (row - firstRow) * cellHeight;

        // Backgrounds with run-length merging.
        auto backgroundColor = [&](const int column) -> QColor {
            const auto& cell = cells[column];
            QColor bg = cell.attributes.background.isValid()
                            ? cell.attributes.background
                            : bgDefault;
            if (cell.attributes.inverse) {
                const QColor fg = cell.attributes.foreground.isValid()
                ? cell.attributes.foreground
                : fgDefault;
                bg = fg;
            }
            if (hasSel) {
                const int linearIndex = row * columns + column;
                if (linearIndex >= selectionStart && linearIndex <= selectionEnd) {
                    bg = selectionColor;
                }
            }
            return bg;
        };

        int runStart = 0;
        QColor runColor = backgroundColor(0);

        for (int column = 1; column < columns; ++column) {
            const QColor bg = backgroundColor(column);
            if (bg != runColor) {
                if (runColor != bgDefault) {
                    painter.fillRect(QRectF(runStart * cellWidth, rowY,
                                            (column - runStart) * cellWidth, cellHeight),
                                     runColor);
                }
                runStart = column;
                runColor = bg;
            }
        }

        if (runColor != bgDefault) {
            painter.fillRect(QRectF(runStart * cellWidth, rowY,
                                    (columns - runStart) * cellWidth, cellHeight),
                             runColor);
        }

        // Build row text and merged formats.
        QString rowText;
        rowText.reserve(columns);

        QVector<QTextLayout::FormatRange> formats;
        formats.reserve(columns);

        QTextCharFormat currentFormat;
        int currentFormatStart = 0;
        int currentFormatLength = 0;
        bool hasFormat = false;

        for (int column = 0; column < columns; ++column) {
            const auto& cell = cells[column];
            const int start = rowText.size();
            rowText += cell.text.isEmpty() ? QLatin1Char(' ') : cell.text;
            const int length = rowText.size() - start;
            if (length <= 0) {
                continue;
            }

            QColor fg = cell.attributes.foreground.isValid()
                            ? cell.attributes.foreground
                            : fgDefault;
            QColor bg = cell.attributes.background.isValid()
                            ? cell.attributes.background
                            : bgDefault;
            if (cell.attributes.inverse) {
                std::swap(fg, bg);
            }

            QTextCharFormat format;
            format.setForeground(fg);
            if (cell.attributes.bold) {
                format.setFontWeight(QFont::Bold);
            }
            if (cell.attributes.underline) {
                format.setFontUnderline(true);
            }

            if (!hasFormat) {
                currentFormat = format;
                currentFormatStart = start;
                currentFormatLength = length;
                hasFormat = true;
            } else if (format == currentFormat) {
                currentFormatLength += length;
            } else {
                formats.append(QTextLayout::FormatRange{currentFormatStart, currentFormatLength, currentFormat});
                currentFormat = format;
                currentFormatStart = start;
                currentFormatLength = length;
            }
        }

        if (hasFormat) {
            formats.append(QTextLayout::FormatRange{currentFormatStart, currentFormatLength, currentFormat});
        }

        if (rowText.isEmpty()) {
            continue;
        }

        QTextLayout layout(rowText, m_terminalFont);
        layout.setTextOption(rowOption);
        layout.beginLayout();

        QTextLine line = layout.createLine();
        if (line.isValid()) {
            line.setLineWidth(columns * cellWidth);
            const qreal padY = qMax<qreal>(0.0, (cellHeight - line.height()) / 2.0);
            line.setPosition(QPointF(0, rowY + padY));
        }
        layout.endLayout();
        layout.draw(&painter, QPointF(0, 0), formats);

        // Cursor
        if (row == cursorVisualRow && cursorColumn >= 0 && line.isValid()) {
            int cursorTextIndex = 0;
            for (int column = 0; column < cursorColumn && column < columns; ++column) {
                const auto& cell = cells[column];
                cursorTextIndex += cell.text.isEmpty() ? 1 : cell.text.size();
            }

            const qreal cursorX = line.cursorToX(cursorTextIndex, QTextLine::Leading);
            const qreal padY = qMax<qreal>(0.0, (cellHeight - line.height()) / 2.0);

            painter.fillRect(QRectF(cursorX, rowY + padY,
                                    qMax(2, cellWidth / 7),
                                    cellHeight),
                             QColor(QStringLiteral("#DEE8FF")));
        }
    }
}

void TerminalView::resizeEvent(QResizeEvent* const event)
{
    QAbstractScrollArea::resizeEvent(event);
    recalculateGrid();
}

void TerminalView::keyPressEvent(QKeyEvent* const event)
{
    const auto modifiers = event->modifiers();

    if (event->matches(QKeySequence::Copy)
        || (event->key() == Qt::Key_C
            && (modifiers & Qt::ControlModifier)
            && (modifiers & Qt::ShiftModifier))) {
        copySelectionToClipboard();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_C && (modifiers & Qt::ControlModifier)) {
        if (hasSelection()) {
            copySelectionToClipboard();
        } else {
            emit terminalInput(QByteArray(1, '\x03'));
        }
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_V
        && (modifiers & Qt::ControlModifier)
        && (modifiers & Qt::ShiftModifier)) {
        emit terminalInput(QApplication::clipboard()->text().toUtf8());
        event->accept();
        return;
    }

    const QByteArray encoded = encodeKey(event);
    if (!encoded.isEmpty()) {
        emit terminalInput(encoded);
        event->accept();
        return;
    }

    QAbstractScrollArea::keyPressEvent(event);
}

void TerminalView::mousePressEvent(QMouseEvent* const event)
{
    if (event->button() == Qt::LeftButton) {
        setFocus(Qt::MouseFocusReason);
        updateSelection(cellAt(event->position().toPoint()), false);
        m_selecting = true;
        event->accept();
        return;
    }
    QAbstractScrollArea::mousePressEvent(event);
}

void TerminalView::mouseMoveEvent(QMouseEvent* const event)
{
    if (m_selecting) {
        updateSelection(cellAt(event->position().toPoint()), true);
        event->accept();
        return;
    }
    QAbstractScrollArea::mouseMoveEvent(event);
}

void TerminalView::mouseReleaseEvent(QMouseEvent* const event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        updateSelection(cellAt(event->position().toPoint()), true);
        event->accept();
        return;
    }
    QAbstractScrollArea::mouseReleaseEvent(event);
}

void TerminalView::focusInEvent(QFocusEvent* const event)
{
    m_hasFocus = true;
    QAbstractScrollArea::focusInEvent(event);
    scheduleViewportUpdate();
}

void TerminalView::focusOutEvent(QFocusEvent* const event)
{
    m_hasFocus = false;
    QAbstractScrollArea::focusOutEvent(event);
    scheduleViewportUpdate();
}

void TerminalView::recalculateGrid()
{
    const int columns = qMax(2, viewport()->width() / m_cellSize.width());
    const int rows = qMax(1, viewport()->height() / m_cellSize.height());

    if (columns != m_screen.columns() || rows != m_screen.rows()) {
        const bool followTail = verticalScrollBar()->value() == verticalScrollBar()->maximum();
        m_screen.resize(columns, rows);
        updateScrollBar(followTail);
        m_resizeDebounce.start();
        scheduleViewportUpdate();
    }
}

void TerminalView::updateScrollBar(const bool followTail)
{
    const int maximum = qMax(0, visualRowCount() - visibleRowCount());
    const int pageStep = visibleRowCount();

    if (verticalScrollBar()->pageStep() != pageStep) {
        verticalScrollBar()->setPageStep(pageStep);
    }

    if (verticalScrollBar()->minimum() != 0 || verticalScrollBar()->maximum() != maximum) {
        verticalScrollBar()->setRange(0, maximum);
    }

    if (followTail) {
        m_scrollOffset = maximum;
        verticalScrollBar()->setValue(maximum);
    } else {
        m_scrollOffset = qBound(0, m_scrollOffset, maximum);
        verticalScrollBar()->setValue(m_scrollOffset);
    }
}

int TerminalView::visibleRowCount() const
{
    return qMax(1, viewport()->height() / m_cellSize.height());
}

int TerminalView::visualRowCount() const
{
    return static_cast<int>(m_screen.scrollback().size() + m_screen.grid().size());
}

const QVector<TerminalScreenModel::Cell>& TerminalView::visualRowAt(const int visualRow) const
{
    const auto& scrollback = m_screen.scrollback();
    const int scrollbackSize = static_cast<int>(scrollback.size());

    if (visualRow < scrollbackSize) {
        return scrollback.at(visualRow);
    }
    return m_screen.grid().at(visualRow - scrollbackSize);
}

void TerminalView::updateSelection(const CellPoint point, const bool extend)
{
    if (point.row < 0 || point.column < 0) {
        return;
    }
    if (!extend) {
        m_selectionAnchor = point;
    }
    m_selectionExtent = point;
    scheduleViewportUpdate();
}

TerminalView::CellPoint TerminalView::cellAt(const QPoint& point) const
{
    const int row = m_scrollOffset + (point.y() / m_cellSize.height());
    const int column = point.x() / m_cellSize.width();

    if (row < 0 || row >= visualRowCount() || column < 0 || column >= m_screen.columns()) {
        return {};
    }
    return {row, column};
}

QByteArray TerminalView::encodeKey(const QKeyEvent* const event) const
{
    const Qt::KeyboardModifiers modifiers = event->modifiers();

    if (!event->text().isEmpty() && !(modifiers & (Qt::AltModifier | Qt::MetaModifier))) {
        if (modifiers & Qt::ControlModifier && event->text().size() == 1) {
            const ushort value = event->text().at(0).toUpper().unicode();
            if (value >= '@' && value <= '_') {
                return QByteArray(1, static_cast<char>(value - '@'));
            }
        }
        return event->text().toUtf8();
    }

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: return QByteArrayLiteral("\r");
    case Qt::Key_Backspace: return QByteArray(1, '\x7f');
    case Qt::Key_Tab: return QByteArrayLiteral("\t");
    case Qt::Key_Escape: return QByteArray(1, '\x1b');
    case Qt::Key_Up: return QByteArrayLiteral("\x1b[A");
    case Qt::Key_Down: return QByteArrayLiteral("\x1b[B");
    case Qt::Key_Right: return QByteArrayLiteral("\x1b[C");
    case Qt::Key_Left: return QByteArrayLiteral("\x1b[D");
    case Qt::Key_Home: return QByteArrayLiteral("\x1b[H");
    case Qt::Key_End: return QByteArrayLiteral("\x1b[F");
    case Qt::Key_Insert: return QByteArrayLiteral("\x1b[2~");
    case Qt::Key_Delete: return QByteArrayLiteral("\x1b[3~");
    case Qt::Key_PageUp: return QByteArrayLiteral("\x1b[5~");
    case Qt::Key_PageDown: return QByteArrayLiteral("\x1b[6~");
    case Qt::Key_F1: return QByteArrayLiteral("\x1bOP");
    case Qt::Key_F2: return QByteArrayLiteral("\x1bOQ");
    case Qt::Key_F3: return QByteArrayLiteral("\x1bOR");
    case Qt::Key_F4: return QByteArrayLiteral("\x1bOS");
    default: return {};
    }
}

void TerminalView::copySelectionToClipboard() const
{
    const QString text = selectedText();
    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
    }
}

bool TerminalView::hasSelection() const
{
    return m_selectionAnchor.row >= 0 && m_selectionExtent.row >= 0;
}

void TerminalView::scheduleViewportUpdate()
{
    if (m_updatePending) {
        return;
    }
    m_updatePending = true;
    m_updateTimer.start();
}

const QColor& TerminalView::defaultForeground()
{
    static const QColor color(QStringLiteral("#DEE8FF"));
    return color;
}

const QColor& TerminalView::defaultBackground()
{
    static const QColor color(QStringLiteral("#03091A"));
    return color;
}