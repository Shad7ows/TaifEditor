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
#include <QListWidget>

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
    void setupLayout();
    void setupStyling();
    void addSettingPage(const QString& name, const QString& description, const QString& iconPath);
    void createAppearancePage(QVBoxLayout*);

    QStackedWidget* stackedWidget{};
    QListWidget* sidebar;

    QSpinBox* fontSpin{};
    QComboBox* fontCombo{};
    QComboBox* themeCombo{};

    QVector<std::shared_ptr<SyntaxTheme>> availableThemes{};
};
