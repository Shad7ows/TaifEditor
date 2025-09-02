#include "SPTerminal.h"

#include <QDockWidget>
#include <QTextBlock>
#include <QKeyEvent>
    
Terminal::Terminal(QWidget* parent)
    : QDockWidget(parent),
    currentHistoryIndex(-1),
    commandStartPosition(0)
{
    setWindowTitle("الطرفية");
    setFont(QFont("Tajawal"));
    setStyleSheet(R"(
        QDockWidget {
            color: #dddddd;
            border: none;
            titlebar-close-icon: url(:/Resources/close.png);
        }
        QDockWidget::title {
            background-color: #1e202e;
            border: none;
            padding: 3px 5px 0 0;
        }
        QDockWidget::close-button {
            icon-size: 10px;
        }
    )");

    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);

    // Create terminal display
    terminalDisplay = new QPlainTextEdit(this);
    setupTerminalDisplay();

    QWidget* dockContent = new QWidget(this);
    QVBoxLayout* vlayTerminal = new QVBoxLayout(dockContent);
    vlayTerminal->setContentsMargins(0, 0, 0, 0);
    vlayTerminal->addWidget(terminalDisplay);

    setWidget(dockContent);

    // Set initial path and prompt
    currentPath = QDir::currentPath();
    insertPrompt(true);

    // Install event filters
    terminalDisplay->installEventFilter(this);
    terminalDisplay->viewport()->installEventFilter(this);
}

void Terminal::setupTerminalDisplay() {
    terminalDisplay->setReadOnly(false);
    terminalDisplay->setStyleSheet(R"(
        QPlainTextEdit {
            background-color: #141520;
            color: white;
            font-family: Consolas, monospace;
        }
    )");
}

bool Terminal::eventFilter(QObject* obj, QEvent* event) {
    if (obj == terminalDisplay and event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // Command history navigation
        if (keyEvent->key() == Qt::Key_Up) {
            navigateCommandHistory(true);
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Down) {
            navigateCommandHistory(false);
            return true;
        }

        // Cursor movement restrictions
        else if (keyEvent->key() == Qt::Key_Left) {
            QTextCursor cursor = terminalDisplay->textCursor();
            if (cursor.position() <= commandStartPosition) {
                return true;  // Block left arrow
            }
        }

        // Enter key handling
        if ((keyEvent->key() == Qt::Key_Enter or keyEvent->key() == Qt::Key_Return) and
            !(keyEvent->modifiers() & Qt::ControlModifier)) {

            QTextCursor cursor = terminalDisplay->textCursor();
            cursor.setPosition(commandStartPosition);
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            QString command = cursor.selectedText().trimmed();

            // Store non-empty commands in history
            if (!command.isEmpty()) {
                if (commandHistory.isEmpty() or commandHistory.last() != command) {
                    commandHistory.append(command);
                }
                currentHistoryIndex = -1;
            }

            // Handle special commands
            if (command.startsWith("cd ")) {
                handleCdCommand(command.mid(3).trimmed());
                return true;
            }

            // Execute command
            if (!command.isEmpty()) {
                executeCommand(command);
                return true;
            }

            // Empty command
            insertPrompt();
            return true;
        }

        // Backspace handling
        else if (keyEvent->key() == Qt::Key_Backspace) {
            QTextCursor cursor = terminalDisplay->textCursor();
            if (cursor.position() <= commandStartPosition) {
                return true;  // Block backspace
            }
        }

    }

    // Mouse click handling
    if (obj == terminalDisplay->viewport()) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        if (event->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::LeftButton) {
            QTextCursor cursor = terminalDisplay->cursorForPosition(mouseEvent->pos());

            // Prevent cursor movement before the command start position
            if (cursor.position() < commandStartPosition) {
                // Instead of just setting cursor position, restore it to the last valid position
                cursor.setPosition(std::max(cursor.position(), commandStartPosition));
                terminalDisplay->setTextCursor(cursor);

                 //QApplication::beep() ; // an audible feedback

                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonDblClick && mouseEvent->button() == Qt::LeftButton) {
            QTextCursor cursor = terminalDisplay->cursorForPosition(mouseEvent->pos());

            // On double-click, move to end of line, but respect commandStartPosition
            cursor.movePosition(QTextCursor::EndOfLine);

            // Ensure cursor doesn't go before commandStartPosition
            if (cursor.position() < commandStartPosition) {
                cursor.setPosition(commandStartPosition);
            }

            terminalDisplay->setTextCursor(cursor);
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}


void Terminal::executeCommand(const QString& command) {
    QString program;
    QStringList arguments;

#ifdef Q_OS_WIN
    program = "cmd.exe";
    arguments << "/c" << command;
#else
    program = "/bin/bash";
    arguments << "-c" << command;
#endif

    QProcess* process = new QProcess(this);
    process->setWorkingDirectory(currentPath);

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        QByteArray output = process->readAllStandardOutput();
        appendColoredText("\n" + QString::fromLocal8Bit(output), Qt::white);
        });

    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        QByteArray error = process->readAllStandardError();
        appendColoredText("\n" + QString::fromLocal8Bit(error), Qt::red);
        });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitStatus == QProcess::CrashExit) {
                appendColoredText("\nProcess crashed\n", Qt::red);
            }
            process->deleteLater();
            insertPrompt();
        }
    );

    process->start(program, arguments);
}

void Terminal::insertPrompt(bool initial) {
    QTextCursor cursor = terminalDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    if (!initial) {
        cursor.insertText("\n");
    }

    setTextColor(Qt::white, &cursor);
    cursor.insertText(QString("%1> ").arg(currentPath));

    commandStartPosition = cursor.position();
    terminalDisplay->setTextCursor(cursor);
}

void Terminal::handleCdCommand(const QString& path) {
    QString newPath = path.startsWith('/') or path.contains(':')
        ? path
        : QDir(currentPath).filePath(path);

    QDir dir(newPath);
    if (dir.exists()) {
        currentPath = QDir::cleanPath(dir.absolutePath());
        QDir::setCurrent(currentPath);
        appendColoredText(QString("\nChanged directory to: %1\n").arg(currentPath), Qt::green);
    }
    else {
        appendColoredText(QString("\nDirectory not found: %1\n").arg(newPath), Qt::red);
    }

    insertPrompt();
}

void Terminal::navigateCommandHistory(bool previous) {
    if (commandHistory.isEmpty()) return;

    // Determine new history index
    if (currentHistoryIndex == -1) {
        currentHistoryIndex = previous ? commandHistory.size() - 1 : 0;
    }
    else {
        currentHistoryIndex = previous
            ? qMax(0, currentHistoryIndex - 1)
            : qMin(commandHistory.size() - 1, currentHistoryIndex + 1);
    }

    // Replace current command
    QTextCursor cursor = terminalDisplay->textCursor();
    cursor.setPosition(commandStartPosition);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    cursor.insertText(commandHistory[currentHistoryIndex]);
}

void Terminal::setTextColor(const QColor& color, QTextCursor* cursor) {
    QTextCharFormat format;
    format.setForeground(color);
    cursor->mergeCharFormat(format);
}

void Terminal::appendColoredText(const QString& text, const QColor& color) {
    QTextCursor cursor = terminalDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat colorFormat;
    colorFormat.setForeground(color);

    cursor.mergeCharFormat(colorFormat);
    cursor.insertText(text);

    terminalDisplay->setTextCursor(cursor);
    terminalDisplay->ensureCursorVisible();
}




