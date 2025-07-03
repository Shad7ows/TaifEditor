#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QCloseEvent>

class SPSettings : public QWidget {
    Q_OBJECT
public:
    explicit SPSettings(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void settingsChanged();
    void windowClosed();

private:
    void loadSettings();
    void saveSettings();
};
