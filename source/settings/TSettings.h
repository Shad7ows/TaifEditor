#include "TFlatButton.h"

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QCloseEvent>
#include <QStackedWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QFontDatabase>
#include <QFormLayout>

class TSettings : public QWidget {
    Q_OBJECT
public:
    explicit TSettings(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void fontSizeChanged(int size);
    // void settingsChanged();
    // void windowClosed();


private:
    void switchPage();
    void createCategory(const QString&, const QString&);
    void createAppearancePage(QVBoxLayout*);

    QVBoxLayout* optionsLayout{};
    QStackedWidget* stackedWidget{};
    QList<TFlatButton*> categories{};

    QSpinBox* fontSpin{};
};
