#pragma once

#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QPair>

class TEditor;

class TPreviewTooltip : public QWidget {
    Q_OBJECT
public:
    explicit TPreviewTooltip(QWidget* parent = nullptr);
    void setContent(const QVector<QPair<int, QString>>& linesContent);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    QVector<QPair<int, QString>> lines{};
    QFont font{};
    int padding = 3;
    int lineSpacing = 1;
    int textWidth = 0;
    int numberWidth = 0;
};

class TMinimap : public QWidget {
    Q_OBJECT
public:
    explicit TMinimap(TEditor* editor, QWidget* parent = nullptr);

public slots:
    void updateMinimap();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void scrollTo(int y);
    void showPreviewTooltip(const QPoint& pos);
    void hidePreviewTooltip();

    void computeLayout(int& outVisibleBlockCount, double& outYRatio,
                       double& outSliderHeight, double& outSliderY) const;

    TEditor* editor{};
    bool isDragging = false;
    bool isHovering = false;
    double clickOffset{};

    static constexpr double MINIMAP_LINE_HEIGHT = 3.0;
    static constexpr int DOT_HEIGHT = 1;

    TPreviewTooltip* previewTooltip{};
    QTimer* previewTimer{};
};