#include "TFlatButton.h"
#include "TSyntaxThemes.h"

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

    QVector<std::shared_ptr<SyntaxTheme>> getAvailableThemes() const;

    QComboBox *getThemeCombo() const;
    void setThemes();

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void fontSizeChanged(int size);
    void fontTypeChanged(QString font);
    void highlighterThemeChanged(int themeIdx);


private:
    void setupStyling();
    void setCategoryActive(TFlatButton* category, bool active);
    void switchPage();
    void createCategory(const QString&, const QString&);
    void createAppearancePage(QVBoxLayout*);

    QVBoxLayout* optionsLayout{};
    QStackedWidget* stackedWidget{};
    QList<TFlatButton*> categories{};

    QSpinBox* fontSpin{};
    QComboBox* fontCombo{};
    QComboBox* themeCombo{};

    QVector<std::shared_ptr<SyntaxTheme>> availableThemes{};
};
