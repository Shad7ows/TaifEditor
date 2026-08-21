#include "TConsole.h"

#include <QApplication>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <QThread>
#include <QVBoxLayout>

namespace {

const QString kPendingTruncationNotice =
    QStringLiteral("\n[تم اختصار جزء من المخرجات بسبب حد المخزن المؤقت]\n");
const QString kRenderedTruncationNotice =
    QStringLiteral("[تم حذف مخرجات أقدم للحفاظ على أداء الطرفية]\n");

} // namespace

TConsole::TConsole(QWidget* const parent)
    : QWidget(parent)
    , m_output(new QPlainTextEdit(this))
    , m_input(new QLineEdit(this))
    , m_process(new QProcess(this))
    , m_flushTimer(new QTimer(this))
{
    m_output->setObjectName(QStringLiteral("ConsoleOutput"));
    m_input->setObjectName(QStringLiteral("ConsoleInput"));
    m_output->setReadOnly(true);
    m_output->setUndoRedoEnabled(false);
    m_output->setWordWrapMode(QTextOption::WordWrap);
    m_output->document()->setMaximumBlockCount(kMaximumRenderedLines);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPixelSize(15);
    m_output->setFont(font);
    m_input->setFont(font);

    setStyleSheet(R"(
        QWidget {
            background-color: #03091A;
            color: #DEE8FF;
        }
    )");

    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_output);
    layout->addWidget(m_input);
    setLayout(layout);

    setConsoleRTL();

    connect(m_process, &QProcess::readyReadStandardOutput, this, &TConsole::processStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &TConsole::processStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TConsole::processFinished);
    connect(m_process, &QProcess::errorOccurred, this, &TConsole::processError);
    connect(m_input, &QLineEdit::returnPressed, this, &TConsole::onInputReturn);

    m_input->installEventFilter(this);

    // The timer stays dormant until output arrives. It coalesces bursts without
    // rebuilding the full QTextDocument every 10 ms while idle.
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(16);
    connect(m_flushTimer, &QTimer::timeout, this, &TConsole::flushPending);
}

TConsole::~TConsole()
{
    stopCmd();
    flushPending();
}

void TConsole::startCmd()
{
    if (m_process->state() != QProcess::NotRunning) {
        return;
    }

#if defined(Q_OS_WIN)
    m_process->start(QStringLiteral("cmd.exe"));
#elif defined(Q_OS_MACOS)
    const QStringList arguments{QStringLiteral("-i"), QStringLiteral("-l")};
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PROMPT_EOL_MARK"), QString());
    m_process->setProcessEnvironment(environment);
    m_process->start(QStringLiteral("zsh"), arguments);
#elif defined(Q_OS_LINUX)
    const QStringList arguments{QStringLiteral("-q"), QStringLiteral("-c"),
                               QStringLiteral("bash"), QStringLiteral("/dev/null")};
    m_process->start(QStringLiteral("script"), arguments);
#endif
}

void TConsole::stopCmd()
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }

    // This method is only used for terminal teardown. Keep the established
    // bounded policy so a console dock can never indefinitely block shutdown.
    m_process->terminate();
    if (!m_process->waitForFinished(500)) {
        m_process->kill();
        m_process->waitForFinished(200);
    }
}

void TConsole::clear()
{
    m_flushTimer->stop();
    m_pendingOutput.clear();
    m_carriageReturnPending = false;
    m_renderedTruncationNoticeVisible = false;
    m_output->clear();
}

void TConsole::setConsoleRTL()
{
    setLayoutDirection(Qt::RightToLeft);
    m_input->setLayoutDirection(Qt::RightToLeft);

    QTextOption option = m_output->document()->defaultTextOption();
    option.setTextDirection(Qt::RightToLeft);
    option.setAlignment(Qt::AlignRight);
    m_output->document()->setDefaultTextOption(option);
}

void TConsole::appendPlainTextThreadSafe(const QString& text)
{
    m_pendingOutput.append(text);
    scheduleFlush();
}

qsizetype TConsole::pendingOutputBytes() const
{
    return m_pendingOutput.pendingBytes();
}

int TConsole::renderedLineCount() const
{
    return m_output->document()->blockCount();
}

qsizetype TConsole::renderedCharacterCount() const
{
    return m_output->document()->characterCount() - 1;
}

void TConsole::processStdout()
{
    appendPlainTextThreadSafe(decodeProcessBytes(m_process->readAllStandardOutput()));
}

void TConsole::processStderr()
{
    appendPlainTextThreadSafe(decodeProcessBytes(m_process->readAllStandardError()));
}

void TConsole::processFinished(const int code, const QProcess::ExitStatus status)
{
    const QString outcome = status == QProcess::NormalExit
        ? QStringLiteral("انتهت العملية بالرمز %1").arg(code)
        : QStringLiteral("انتهت العملية بشكل غير طبيعي (الرمز %1)").arg(code);
    appendPlainTextThreadSafe(QStringLiteral("\n[%1]\n").arg(outcome));
}

void TConsole::processError(const QProcess::ProcessError error)
{
    if (error == QProcess::UnknownError) {
        return;
    }
    appendPlainTextThreadSafe(
        QStringLiteral("\n[خطأ في الطرفية: %1]\n").arg(m_process->errorString()));
}

void TConsole::onInputReturn()
{
    const QString command = m_input->text();
    if (m_history.isEmpty() || m_history.last() != command) {
        m_history.append(command);
    }
    m_historyIndex = -1;

#if defined(Q_OS_WIN)
    appendPlainTextThreadSafe(command + QStringLiteral("\n"));
    if (m_process->state() != QProcess::NotRunning) {
        m_process->write((command + QStringLiteral("\r\n")).toLocal8Bit());
    }
#else
    if (m_process->state() != QProcess::NotRunning) {
        m_process->write((command + QStringLiteral("\n")).toUtf8());
    }
#endif

    emit commandEntered(command);
    m_input->clear();
}

void TConsole::flushPending()
{
    const OutputBuffer::DrainResult result = m_pendingOutput.drain();
    if (result.text.isEmpty() && !result.truncated) {
        return;
    }

    if (result.truncated) {
        emit outputTruncated(result.droppedBytes);
        appendRenderedText(kPendingTruncationNotice);
    }
    appendRenderedText(result.text);
}

void TConsole::scheduleFlush()
{
    if (QThread::currentThread() == thread()) {
        if (!m_flushTimer->isActive()) {
            m_flushTimer->start();
        }
        return;
    }

    QMetaObject::invokeMethod(this, [this]() {
        if (!m_flushTimer->isActive()) {
            m_flushTimer->start();
        }
    }, Qt::QueuedConnection);
}

void TConsole::appendRenderedText(const QString& text)
{
    if (text.isEmpty()) {
        return;
    }

    const bool followOutput = isAtBottom();
    QTextCursor cursor(m_output->document());
    cursor.movePosition(QTextCursor::End);

    qsizetype position = 0;
    while (position < text.size()) {
        const qsizetype carriageReturn = text.indexOf(QLatin1Char('\r'), position);
        const qsizetype lineFeed = text.indexOf(QLatin1Char('\n'), position);
        qsizetype control = -1;
        if (carriageReturn >= 0 && lineFeed >= 0) {
            control = qMin(carriageReturn, lineFeed);
        } else if (carriageReturn >= 0) {
            control = carriageReturn;
        } else {
            control = lineFeed;
        }

        const qsizetype textEnd = control >= 0 ? control : text.size();
        if (textEnd > position) {
            if (m_carriageReturnPending) {
                cursor.movePosition(QTextCursor::EndOfBlock);
                cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                m_carriageReturnPending = false;
            }
            cursor.movePosition(QTextCursor::End);
            cursor.insertText(text.mid(position, textEnd - position));
        }
        if (control < 0) {
            break;
        }

        if (text.at(control) == QLatin1Char('\r')) {
            m_carriageReturnPending = true;
        } else {
            cursor.movePosition(QTextCursor::End);
            cursor.insertBlock();
            m_carriageReturnPending = false;
        }
        position = control + 1;
    }

    trimRenderedOutput();
    if (followOutput) {
        m_output->verticalScrollBar()->setValue(m_output->verticalScrollBar()->maximum());
    }
}

void TConsole::trimRenderedOutput()
{
    QTextDocument* const document = m_output->document();
    const qsizetype renderedCharacters = document->characterCount() - 1;
    if (renderedCharacters <= kMaximumRenderedCharacters) {
        return;
    }

    const qsizetype excess = renderedCharacters - kMaximumRenderedCharacters;
    QTextCursor cursor(document);
    cursor.setPosition(0);
    cursor.setPosition(static_cast<int>(qMin<qsizetype>(excess, renderedCharacters)),
                       QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    if (!m_renderedTruncationNoticeVisible) {
        cursor.setPosition(0);
        cursor.insertText(kRenderedTruncationNotice);
        m_renderedTruncationNoticeVisible = true;
    }
}

QString TConsole::decodeProcessBytes(const QByteArray& bytes) const
{
    // Child shells follow the native Windows code page; Unix shells and Alif
    // tools use UTF-8. Decoding is intentionally performed before chunks reach
    // OutputBuffer so the buffer never stores partial byte sequences.
#if defined(Q_OS_WIN)
    return QString::fromLocal8Bit(bytes);
#else
    return QString::fromUtf8(bytes);
#endif
}

bool TConsole::isAtBottom() const
{
    const QScrollBar* const scrollBar = m_output->verticalScrollBar();
    return scrollBar->value() >= scrollBar->maximum();
}

bool TConsole::eventFilter(QObject* const watched, QEvent* const event)
{
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        auto* const keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up) {
            if (m_history.isEmpty()) {
                return true;
            }
            if (m_historyIndex == -1) {
                m_historyIndex = m_history.size() - 1;
            } else {
                m_historyIndex = qMax(0, m_historyIndex - 1);
            }
            m_input->setText(m_history.at(m_historyIndex));
            return true;
        }
        if (keyEvent->key() == Qt::Key_Down) {
            if (m_history.isEmpty() || m_historyIndex == -1) {
                return true;
            }
            m_historyIndex = qMin(m_history.size() - 1, m_historyIndex + 1);
            m_input->setText(m_history.at(m_historyIndex));
            return true;
        }
        if (keyEvent->matches(QKeySequence::Copy)) {
            return QWidget::eventFilter(watched, event);
        }
        if (keyEvent->key() == Qt::Key_C && (keyEvent->modifiers() & Qt::ControlModifier)) {
            return false;
        }
        if (keyEvent->key() == Qt::Key_L && (keyEvent->modifiers() & Qt::ControlModifier)) {
            clear();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Tab) {
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
