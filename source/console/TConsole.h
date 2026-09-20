#pragma once

#include "OutputBuffer.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QTimer>
#include <QVector>
#include <QWidget>

class TConsole : public QWidget {
    Q_OBJECT
public:
    static constexpr int kMaximumRenderedLines = 2000;
    static constexpr qsizetype kMaximumRenderedCharacters = 512 * 1024;

    explicit TConsole(QWidget* parent = nullptr);
    ~TConsole() override;

    // Interactive system-shell actions. Alif execution is deliberately handled
    // by AlifRunController and only streams its output into another TConsole.
    void startCmd();
    void stopCmd();
    void clear();
    void setConsoleRTL();
    void appendPlainTextThreadSafe(const QString& text);

    [[nodiscard]] qsizetype pendingOutputBytes() const;
    [[nodiscard]] int renderedLineCount() const;
    [[nodiscard]] qsizetype renderedCharacterCount() const;

signals:
    void commandEntered(const QString& command);
    void outputTruncated(qsizetype droppedPendingBytes);

private slots:
    void processStdout();
    void processStderr();
    void processFinished(int code, QProcess::ExitStatus status);
    void processError(QProcess::ProcessError error);
    void onInputReturn();
    void flushPending();

private:
    void scheduleFlush();
    void appendRenderedText(const QString& text);
    void trimRenderedOutput();
    [[nodiscard]] QString decodeProcessBytes(const QByteArray& bytes) const;
    [[nodiscard]] bool isAtBottom() const;
    bool eventFilter(QObject* watched, QEvent* event) override;

    QPlainTextEdit* m_output{};
    QLineEdit* m_input{};
    QProcess* m_process{};
    QTimer* m_flushTimer{};
    bool m_carriageReturnPending = false;

    OutputBuffer m_pendingOutput;

    QVector<QString> m_history;
    int m_historyIndex = -1;
    bool m_autoscroll = true;
    bool m_renderedTruncationNoticeVisible = false;
};
