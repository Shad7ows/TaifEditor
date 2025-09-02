#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QProcess>
#include <QDir>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QPlainTextEdit>

class Terminal : public QDockWidget {
    Q_OBJECT

public:
    explicit Terminal(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QPlainTextEdit* terminalDisplay;
    QStringList commandHistory;
    int currentHistoryIndex;
    QString currentPath;
    int commandStartPosition;

    void setupTerminalDisplay();
    void executeCommand(const QString& command);
    void insertPrompt(bool initial = false);
    void handleCdCommand(const QString& path);
    void navigateCommandHistory(bool previous);
    void setTextColor(const QColor& color, QTextCursor* cursor);
    void appendColoredText(const QString& text, const QColor& color);
};