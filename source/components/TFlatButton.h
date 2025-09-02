#include <QPushButton>
#include <QPainter>
#include <QPropertyAnimation>
#include <QEnterEvent>
#include <QFont>

class TFlatButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity CONSTANT)

public:
    explicit TFlatButton(QWidget *parent = nullptr, QString lable = "");

    qreal glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(qreal intensity) {
        m_glowIntensity = intensity;
        update();
    }

protected:
    void enterEvent(QEnterEvent *event) override;

    void leaveEvent(QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

private:
    void startHoverAnimation(qreal);

    qreal m_glowIntensity{};
};
