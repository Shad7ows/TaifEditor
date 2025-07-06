#include "FlatButton.h"

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

class SPSettings : public QWidget {
    Q_OBJECT
public:
    explicit SPSettings(QWidget* parent = nullptr);

protected:
    // void closeEvent(QCloseEvent* event) override;

signals:
    void settingsChanged();
    void windowClosed();

private:
    void switchPage();
    void createCategory(const QString&, const QString&);
    void createAppearancePage(QVBoxLayout*);

    QVBoxLayout* optionsLayout{};
    QStackedWidget* stackedWidget{};
    QList<SPFlatButton*> categories{};
};
