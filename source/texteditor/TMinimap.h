#pragma once

#include <QWidget>
#include <QPainter>
#include <QTextBlock>
#include <QMouseEvent>
#include <QScrollBar>
#include <QLabel>
#include <QTimer>
#include <QPlainTextEdit>

class TEditor;

// أداة خريطة الكود (Minimap)
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
    void leaveEvent(QEvent* event) override;

private:
    void scrollTo(int y);
    void computeLayout(int& outVisibleBlockCount, double& outYRatio,
                       double& outSliderHeight, double& outSliderY);
    void showPreviewTooltip(const QPoint& pos);
    void hidePreviewTooltip();

    TEditor* editor;
    bool isDragging;
    double clickOffset; // لحفظ فرق المسافة عند الضغط على المربع
    const double MINIMAP_LINE_HEIGHT = 2.0; // ارتفاع كل سطر في الخريطة المصغرة

    // نافذة المعاينة عند التمرير
    QLabel* previewLabel{};
    QTimer* previewTimer{};
    bool isHovering{};
};
