#include "TConsole.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QTextBlockFormat>
#include <QApplication>
#include <QTextBlock>


TConsole::TConsole(QWidget *parent)
    : QWidget(parent),
    m_output(new QPlainTextEdit(this)),
    m_input(new QLineEdit(this)),
    m_process(new QProcess(this)),
    m_flushTimer(new QTimer(this)),
    m_historyIndex(-1),
    m_autoscroll(true)
{
    // UI
    m_output->setReadOnly(true);
    m_output->setUndoRedoEnabled(false);
    m_output->setWordWrapMode(QTextOption::WordWrap);
    // simple monospace font
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPixelSize(15);
    m_output->setFont(f);
    m_input->setFont(f);

    setStyleSheet("QWidget {background-color: #03091A; color: #DEE8FF;}");

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);
    lay->addWidget(m_output);
    lay->addWidget(m_input);
    setLayout(lay);

    setLayoutDirection(Qt::RightToLeft);
    m_input->setLayoutDirection(Qt::RightToLeft);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &TConsole::processStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &TConsole::processStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TConsole::processFinished);

    connect(m_input, &QLineEdit::returnPressed, this, &TConsole::onInputReturn);

    m_input->installEventFilter(this);

    m_flushTimer->setInterval(10);
    connect(m_flushTimer, &QTimer::timeout, this, &TConsole::flushPending);
    m_flushTimer->start();
}

TConsole::~TConsole()
{
    stopCmd();
}

void TConsole::startCmd()
{
    if (m_process->state() != QProcess::NotRunning) return;

#if defined(Q_OS_WIN)
    m_process->start("cmd.exe");
#elif defined(Q_OS_MACOS)
    QStringList args{};
    args << "-i" << "-l"; // mean (interactive)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PROMPT_EOL_MARK", ""); // لإزاله علامة "٪" من نهاية السطر الجديد في الطرفية
    m_process->setProcessEnvironment(env);
    m_process->start("zsh", args);
#elif defined(Q_OS_LINUX)
    QStringList args{};
    args << "-q" << "-c" << "bash" << "/dev/null"; // mean (interactive)
    m_process->start("script", args);
#endif
}

void TConsole::stopCmd()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(500)) {
            m_process->kill();
            m_process->waitForFinished(200);
        }
    }
}

void TConsole::clear()
{
    m_output->clear();
    m_pending.clear();
    m_buffer.clear();
}

void TConsole::setConsoleRTL()
{
    setLayoutDirection(Qt::RightToLeft);

    QTextOption opt = m_output->document()->defaultTextOption();
    opt.setTextDirection(Qt::RightToLeft);
    opt.setAlignment(Qt::AlignRight);
    m_output->document()->setDefaultTextOption(opt);
}

void TConsole::appendPlainTextThreadSafe(const QString &text)
{
    QMutexLocker locker(&m_pendingMutex);
    m_pending.append(text);
    locker.unlock();
}

void TConsole::processStdout()
{
    QByteArray d = m_process->readAllStandardOutput();
    QString s;
#if defined(Q_OS_WIN)
    s = QString::fromLocal8Bit(d);
#else
    s = QString::fromUtf8(d);
#endif
    appendPlainTextThreadSafe(s);
}

void TConsole::processStderr()
{
    QByteArray d = m_process->readAllStandardError();
    QString s = QString::fromLocal8Bit(d);
    appendPlainTextThreadSafe(s);
}

void TConsole::processFinished(int code, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    appendPlainTextThreadSafe(QString("\n[العملية إنتهت بالرمز %1]\n").arg(code));
}

void TConsole::onInputReturn()
{
    QString cmd = m_input->text();
    if (cmd.isEmpty()) {
        if (m_process->state() != QProcess::NotRunning) {
#if defined(Q_OS_WIN)
            m_process->write("\r\n");
#else
            m_process->write("\n");
#endif
        }
    }

    if (m_history.isEmpty() || m_history.last() != cmd) {
        m_history.append(cmd);
    }
    m_historyIndex = -1;

// echo the command locally (like terminal)
#if defined(Q_OS_WIN)
    appendPlainTextThreadSafe(cmd + "\n");
#endif

    // send to process (CRLF on Windows)
#if defined(Q_OS_WIN)
    if (m_process->state() != QProcess::NotRunning) {
        m_process->write((cmd + "\r\n").toLocal8Bit());
    }
#else
    if (m_process->state() != QProcess::NotRunning) {
        m_process->write((cmd + "\n").toLocal8Bit());
    }
#endif

    emit commandEntered(cmd);
    m_input->clear();
}

void TConsole::flushPending() {
    QStringList items;
    {
        QMutexLocker locker(&m_pendingMutex);
        if (m_pending.isEmpty()) return;
        items = m_pending;
        m_pending.clear();
    }

    // Ensure we have at least one empty line to work with
    if (m_buffer.isEmpty()) {
        m_buffer.append("");
    }

    for (const QString &chunk : items) {
        for (int i = 0; i < chunk.length(); ++i) {
            QChar c = chunk[i];

            if (c == '\r') {
                // Check if this is a Windows style \r\n
                if (i + 1 < chunk.length() && chunk[i + 1] == '\n') {
                    // Skip the \r and let the \n handle the line break next iteration
                    continue;
                } else {
                    // Independent \r: Move "cursor" to start of line.
                    // In a simple buffer, this effectively means clearing the current line
                    // so the next characters overwrite it.
                    m_buffer.last().clear();
                }
            }
            else if (c == '\n') {
                // New line: finalize the current line and move to next
                m_buffer.append("");
            }
            else {
                // Regular character: append to the current active line
                m_buffer.last().append(c);
            }
        }
    }

    // Keep buffer within limits
    while (m_buffer.size() > m_maxLines) {
        m_buffer.pop_front();
    }

    // Update the UI
    m_output->setPlainText(m_buffer.join("\n"));

    if (m_autoscroll) {
        QScrollBar *sb = m_output->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
}

bool TConsole::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_input && ev->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(ev);
        if (ke->key() == Qt::Key_Up) {
            if (m_history.isEmpty()) return true;
            if (m_historyIndex == -1) m_historyIndex = m_history.size() - 1;
            else m_historyIndex = qMax(0, m_historyIndex - 1);
            m_input->setText(m_history[m_historyIndex]);
            return true;
        } else if (ke->key() == Qt::Key_Down) {
            if (m_history.isEmpty()) return true;
            if (m_historyIndex == -1) return true;
            m_historyIndex = qMin(m_history.size() - 1, m_historyIndex + 1);
            if (m_historyIndex >= 0 && m_historyIndex < m_history.size())
                m_input->setText(m_history[m_historyIndex]);
            else
                m_input->clear();
            return true;
        } else if (ke->matches(QKeySequence::Copy)) {
            return QWidget::eventFilter(obj, ev);
        } else if (ke->key() == Qt::Key_C && (ke->modifiers() & Qt::ControlModifier)) {
            return false;
        } else if (ke->key() == Qt::Key_L && (ke->modifiers() & Qt::ControlModifier)) {
            clear();
            return true;
        } else if (ke->key() == Qt::Key_Tab) {
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}

