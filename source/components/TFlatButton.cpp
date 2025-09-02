#include "TFlatButton.h"


TFlatButton::TFlatButton(QWidget *parent, QString lable) {
    setFlat(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(30);
    setFont(QFont("Tajawal", 14, QFont::Thin));
    setCursor(Qt::PointingHandCursor);
    setText(lable);
}

void TFlatButton::enterEvent(QEnterEvent *event) {
    QPushButton::enterEvent(event);
    startHoverAnimation(1.0);
}

void TFlatButton::leaveEvent(QEvent *event) {
    QPushButton::leaveEvent(event);
    startHoverAnimation(0.0);
}

void TFlatButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background with animated glow effect
    QColor bgColor = palette().color(QPalette::Window);
    bgColor.setAlphaF(0.3 * m_glowIntensity);

    QLinearGradient gradient(rect().topLeft(), rect().topRight());
    gradient.setColorAt(0, QColor(64, 140, 200, 150 * m_glowIntensity));
    gradient.setColorAt(1, QColor(120, 180, 245, 80 * m_glowIntensity));

    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawRect(rect());

    // Draw creative elements
    painter.setPen(QPen(QColor(180, 220, 255, 200 * m_glowIntensity), 2));
    painter.drawLine(rect().left(), rect().bottom(),
                     rect().right(), rect().bottom());

    // Draw text with right alignment
    painter.setPen(palette().color(QPalette::ButtonText));
    QRect textRect = rect().adjusted(50, 0, -15, 0);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());

    // Draw arrow indicator
    if(m_glowIntensity > 0.1) {
        painter.setPen(QPen(QColor(255, 255, 255, 200 * m_glowIntensity), 2));
        QPointF arrow[3] = {
            QPointF(37, height()/2 + 6),
            QPointF(30, height()/2),    // Arrow tip
            QPointF(37, height()/2 - 6),
        };
        painter.drawPolyline(arrow, 3);
    }
}


void TFlatButton::startHoverAnimation(qreal targetValue) {
    QPropertyAnimation *anim = new QPropertyAnimation(this, "glowIntensity");
    anim->setDuration(300);
    anim->setStartValue(m_glowIntensity);
    anim->setEndValue(targetValue);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start(QPropertyAnimation::DeleteWhenStopped);
}
