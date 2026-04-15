#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>

class TEditor;

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
    double clickOffset = 0.0;

    // `constexpr` ensures this is evaluated at compile time
    static constexpr double MINIMAP_LINE_HEIGHT = 3.0;
    static constexpr int DOT_HEIGHT = 1;

    QLabel* previewLabel{};
    QTimer* previewTimer{};
};