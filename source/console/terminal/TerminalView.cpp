#include "TerminalView.h"

#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

namespace {

bool isSpecialKey(const int key)
{
    return key >= Qt::Key_Escape && key <= Qt::Key_F35;
}

} // namespace

TerminalView::TerminalView(QWidget* const parent)
    : QAbstractScrollArea(parent)
    , m_screen(80, 24)
    , m_parser(m_screen)
    , m_resizeDebounce(this)
{
    setObjectName(QStringLiteral("NativeTerminalView"));
    setAccessibleName(QStringLiteral("عرض الطرفية الأصلية"));
    setFocusPolicy(Qt::StrongFocus);
    setLayoutDirection(Qt::LeftToRight);
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
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](const int value) {
        m_scrollOffset = value;
        viewport()->update();
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
    viewport()->update();
}

void TerminalView::clearTerminal()
{
    m_screen.clearAll();
    m_selectionAnchor = {};
    m_selectionExtent = {};
    updateScrollBar(true);
    viewport()->update();
}

TerminalScreenModel& TerminalView::screen() { return m_screen; }
const TerminalScreenModel& TerminalView::screen() const { return m_screen; }
QSize TerminalView::gridSize() const { return QSize(m_screen.columns(), m_screen.rows()); }

QString TerminalView::selectedText() const
{
    if (m_selectionAnchor.row < 0 || m_selectionExtent.row < 0) {
        return {};
    }
    const int begin = qMin(m_selectionAnchor.row * m_screen.columns() + m_selectionAnchor.column,
                           m_selectionExtent.row * m_screen.columns() + m_selectionExtent.column);
    const int end = qMax(m_selectionAnchor.row * m_screen.columns() + m_selectionAnchor.column,
                         m_selectionExtent.row * m_screen.columns() + m_selectionExtent.column);
    QString result;
    for (int index = begin; index <= end; ++index) {
        const int row = index / m_screen.columns();
        const int column = index % m_screen.columns();
        if (row >= 0 && row < visualRowCount()) {
            result += visualRowAt(row).at(column).text;
            if (column == m_screen.columns() - 1 && row != end / m_screen.columns()) {
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
    painter.fillRect(viewport()->rect(), defaultBackground());
    painter.setFont(m_terminalFont);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const int totalRows = visualRowCount();
    const int visibleRows = visibleRowCount();
    const int firstRow = qBound(0, m_scrollOffset, qMax(0, totalRows - visibleRows));
    const int lastRow = qMin(totalRows, firstRow + visibleRows);
    const int selectionStart = (m_selectionAnchor.row < 0 || m_selectionExtent.row < 0)
        ? -1 : qMin(m_selectionAnchor.row * m_screen.columns() + m_selectionAnchor.column,
                    m_selectionExtent.row * m_screen.columns() + m_selectionExtent.column);
    const int selectionEnd = (m_selectionAnchor.row < 0 || m_selectionExtent.row < 0)
        ? -1 : qMax(m_selectionAnchor.row * m_screen.columns() + m_selectionAnchor.column,
                    m_selectionExtent.row * m_screen.columns() + m_selectionExtent.column);

    for (int row = firstRow; row < lastRow; ++row) {
        for (int column = 0; column < m_screen.columns(); ++column) {
            const TerminalScreenModel::Cell& cell = visualRowAt(row).at(column);
            QRect cellRect((column * m_cellSize.width()),
                           ((row - firstRow) * m_cellSize.height()),
                           m_cellSize.width(), m_cellSize.height());
            const int linearIndex = row * m_screen.columns() + column;
            const bool selected = selectionStart >= 0 && linearIndex >= selectionStart && linearIndex <= selectionEnd;
            QColor background = cell.attributes.background.isValid() ? cell.attributes.background : defaultBackground();
            QColor foreground = cell.attributes.foreground.isValid() ? cell.attributes.foreground : defaultForeground();
            if (cell.attributes.inverse) {
                std::swap(background, foreground);
            }
            if (selected) {
                background = QColor(QStringLiteral("#294b78"));
            }
            painter.fillRect(cellRect, background);
            QFont cellFont = m_terminalFont;
            cellFont.setBold(cell.attributes.bold);
            cellFont.setUnderline(cell.attributes.underline);
            painter.setFont(cellFont);
            painter.setPen(foreground);
            painter.drawText(cellRect.adjusted(0, 0, 0, -1), Qt::AlignLeft | Qt::AlignVCenter,
                             cell.text);
        }
    }

    if (m_hasFocus && m_screen.cursor().visible && (m_cursorClock.elapsed() / 500) % 2 == 0
        && m_scrollOffset == verticalScrollBar()->maximum()) {
        const TerminalScreenModel::Cursor cursor = m_screen.cursor();
        const int cursorVisualRow = m_screen.scrollback().size() + cursor.row;
        const int visibleCursorRow = cursorVisualRow - firstRow;
        if (visibleCursorRow >= 0 && visibleCursorRow < visibleRows) {
            painter.fillRect(QRect(cursor.column * m_cellSize.width(),
                                   visibleCursorRow * m_cellSize.height(),
                                   qMax(2, m_cellSize.width() / 7), m_cellSize.height()),
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
    if (event->matches(QKeySequence::Copy) || (event->key() == Qt::Key_C
        && (event->modifiers() & Qt::ControlModifier) && !selectedText().isEmpty())) {
        copySelectionToClipboard();
        event->accept();
        return;
    }
    if ((event->key() == Qt::Key_C && (event->modifiers() & Qt::ControlModifier))
        || (event->key() == Qt::Key_C && (event->modifiers() & Qt::ControlModifier)
            && (event->modifiers() & Qt::ShiftModifier))) {
        emit terminalInput(QByteArray(1, '\x03'));
        event->accept();
        return;
    }
    if ((event->key() == Qt::Key_V && (event->modifiers() & Qt::ControlModifier)
         && (event->modifiers() & Qt::ShiftModifier))) {
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
    viewport()->update();
}

void TerminalView::focusOutEvent(QFocusEvent* const event)
{
    m_hasFocus = false;
    QAbstractScrollArea::focusOutEvent(event);
    viewport()->update();
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
        viewport()->update();
    }
}

void TerminalView::updateScrollBar(const bool followTail)
{
    const int maximum = qMax(0, visualRowCount() - visibleRowCount());
    verticalScrollBar()->setPageStep(visibleRowCount());
    verticalScrollBar()->setRange(0, maximum);
    if (followTail) {
        verticalScrollBar()->setValue(maximum);
    }
}

int TerminalView::visibleRowCount() const
{
    return qMax(1, viewport()->height() / m_cellSize.height());
}

int TerminalView::visualRowCount() const
{
    return m_screen.scrollback().size() + m_screen.grid().size();
}

const QVector<TerminalScreenModel::Cell>& TerminalView::visualRowAt(const int visualRow) const
{
    const auto& scrollback = m_screen.scrollback();
    if (visualRow < scrollback.size()) {
        return scrollback.at(visualRow);
    }
    return m_screen.grid().at(visualRow - scrollback.size());
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
    viewport()->update();
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

QColor TerminalView::defaultForeground() const { return QColor(QStringLiteral("#DEE8FF")); }
QColor TerminalView::defaultBackground() const { return QColor(QStringLiteral("#03091A")); }
