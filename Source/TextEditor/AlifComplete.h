#pragma once

#include <QObject>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QStringList>

class AutoComplete : public QObject
{
    Q_OBJECT
public:
    explicit AutoComplete(QPlainTextEdit* editor, QObject* parent = nullptr);

    bool isPopupVisible();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void showCompletion();
    void insertCompletion();

private:
    QPlainTextEdit* editor{};
    QWidget* popup{};
    QListWidget* listWidget{};
    QStringList keywords{};
    QMap<QString, QString> shortcuts;
    QMap<QString, QString> descriptions;
    QList<int> placeholderPositions;

    QString getCurrentWord() const;
    void showPopup();
    inline void hidePopup();
};
