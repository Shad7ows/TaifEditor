#pragma once

#include "TerminalScreenModel.h"
#include "VtStreamParser.h"

#include <QAbstractScrollArea>
#include <QElapsedTimer>
#include <QTimer>

/** LTR cell-grid terminal viewport, independent from the surrounding RTL UI. */
class TerminalView final : public QAbstractScrollArea {
    Q_OBJECT
public:
    explicit TerminalView(QWidget* parent = nullptr);

    void appendOutput(const QByteArray& bytes);
    void clearTerminal();
    [[nodiscard]] TerminalScreenModel& screen();
    [[nodiscard]] const TerminalScreenModel& screen() const;
    [[nodiscard]] QSize gridSize() const;
    [[nodiscard]] QString selectedText() const;

signals:
    void terminalInput(const QByteArray& bytes);
    void gridSizeChanged(const QSize& cells);
    void terminalTitleChanged(const QString& title);

protected:
    bool viewportEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    struct CellPoint final { int row = -1; int column = -1; };

    void recalculateGrid();
    void updateScrollBar(bool followTail);
    [[nodiscard]] int visibleRowCount() const;
    [[nodiscard]] int visualRowCount() const;
    [[nodiscard]] const QVector<TerminalScreenModel::Cell>& visualRowAt(int visualRow) const;
    void updateSelection(CellPoint point, bool extend);
    [[nodiscard]] CellPoint cellAt(const QPoint& point) const;
    [[nodiscard]] QByteArray encodeKey(const QKeyEvent* event) const;
    void copySelectionToClipboard() const;
    [[nodiscard]] QColor defaultForeground() const;
    [[nodiscard]] QColor defaultBackground() const;

    TerminalScreenModel m_screen;
    VtStreamParser m_parser;
    QFont m_terminalFont;
    QSize m_cellSize;
    int m_scrollOffset = 0;
    CellPoint m_selectionAnchor;
    CellPoint m_selectionExtent;
    bool m_selecting = false;
    bool m_hasFocus = false;
    QTimer m_resizeDebounce;
    QElapsedTimer m_cursorClock;
};
