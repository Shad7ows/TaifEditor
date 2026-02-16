#include "TEditor.h"

#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QMimeData>
#include <QSettings>
#include <QPainterPath>
#include <QStack>
#include <QMenu>
#include <QAction>
#include <QFile>


TEditor::TEditor(TSettings* setting, QWidget* parent) {
    setAcceptDrops(true);
    this->setStyleSheet("QPlainTextEdit { background-color: #141520; color: #cccccc; }");
    this->setTabStopDistance(32);

    QTextDocument* editorDocument = this->document();
    QTextOption option = editorDocument->defaultTextOption();
    option.setTextDirection(Qt::RightToLeft);
    option.setAlignment(Qt::AlignRight);
    editorDocument->setDefaultTextOption(option);


    highlighter = new TSyntaxHighlighter(editorDocument);
    lineNumberArea = new LineNumberArea(this);

    // ضبط الإكمال التلقائي
    setupAutoComplete();

    connect(this, &TEditor::blockCountChanged, this, &TEditor::updateLineNumberAreaWidth);
    connect(this, &TEditor::updateRequest, this, &TEditor::updateLineNumberArea);
    connect(this, &TEditor::cursorPositionChanged, this, &TEditor::highlightCurrentLine);
    connect(this->document(), &QTextDocument::contentsChanged, this, &TEditor::updateFoldRegions);

    updateLineNumberAreaWidth();
    highlightCurrentLine();

    // set saved setting font size to the editor
    QSettings settingsVal("Alif", "Taif");
    int savedSize = settingsVal.value("editorFontSize").toInt();
    updateFontSize(savedSize);
    // set saved setting font type to the editor
    QString savedFont = settingsVal.value("editorFontType").toString();
    updateFontType(savedFont);
    // set saved setting theme to the editor
    int savedTheme = settingsVal.value("editorCodeTheme").toInt();
    savedTheme >= 0 ? savedTheme : savedTheme = 0;
    std::shared_ptr<SyntaxTheme> theme = setting->getAvailableThemes().at(savedTheme);
    updateHighlighterTheme(theme);

    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setInterval(60000);
    connect(autoSaveTimer, &QTimer::timeout, this, &TEditor::performAutoSave);

    connect(this->document(), &QTextDocument::contentsChanged, this, &TEditor::startAutoSave);

    installEventFilter(this);
}

void TEditor::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta == 0) return;

        QFont font = this->font();
        qreal currentSize = font.pointSizeF();

        qreal step = 0.5;

        if (delta > 0) {
            currentSize += step;
        } else {
            currentSize -= step;
        }

        if (currentSize < 5.0) currentSize = 5.0;
        if (currentSize > 50) currentSize = 50;

        font.setPointSizeF(currentSize);
        this->setFont(font);

        if (lineNumberArea) {
            QFont lineFont = lineNumberArea->font();
            lineFont.setPointSizeF(currentSize);
            lineNumberArea->setFont(lineFont);
        }

        updateLineNumberAreaWidth();

        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void TEditor::updateFontSize(int size) {
    if (size < 10) {
        size = 18;
    }

    QFont font = this->font();
    font.setPixelSize(size);
    this->setFont(font);

    QFont fontNums = lineNumberArea->font();
    fontNums.setPixelSize(size);
    lineNumberArea->setFont(fontNums);
}

void TEditor::updateFontType(QString font) {
    QFont currentFont = this->font();
    currentFont.setFamily(font);

    this->setFont(currentFont);
}


// 1. دالة تعليق/إلغاء تعليق الأكواد
void TEditor::toggleComment()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock(); // لبدء عملية تراجع (Undo) واحدة

    int startPos = cursor.selectionStart();
    int endPos = cursor.selectionEnd();

    // تحديد بداية ونهاية الأسطر المحددة
    cursor.setPosition(startPos);
    int startBlock = cursor.blockNumber();
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    int endBlock = cursor.blockNumber();

    if (cursor.atBlockStart() && endBlock > startBlock) {
        endBlock--;
    }

    bool shouldComment = false;

    QTextBlock block = document()->findBlockByNumber(startBlock);
    if (!block.text().trimmed().startsWith("#")) {
        shouldComment = true;
    }

    for (int i = startBlock; i <= endBlock; ++i) {
        block = document()->findBlockByNumber(i);
        QTextCursor lineCursor(block);

        if (shouldComment) {
            lineCursor.movePosition(QTextCursor::StartOfBlock);
            lineCursor.insertText("#");
        } else {
            QString text = block.text();
            int idx = text.indexOf("#");
            lineCursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, idx);
            lineCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            lineCursor.removeSelectedText();
        }
    }

    cursor.endEditBlock();
}

void TEditor::duplicateLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    QString lineText = cursor.block().text();

    cursor.movePosition(QTextCursor::EndOfBlock);

    cursor.insertText("\n" + lineText);

    cursor.endEditBlock();
}

void TEditor::moveLineUp()
{
    QTextCursor cursor = textCursor();
    QTextBlock currentBlock = cursor.block();
    QTextBlock prevBlock = currentBlock.previous();

    if (!prevBlock.isValid()) return;

    cursor.beginEditBlock();

    QString currentText = currentBlock.text();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.deletePreviousChar();

    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertText(currentText + "\n");

    cursor.movePosition(QTextCursor::Up);
    setTextCursor(cursor);

    cursor.endEditBlock();
}

void TEditor::moveLineDown()
{
    QTextCursor cursor = textCursor();
    QTextBlock currentBlock = cursor.block();
    QTextBlock nextBlock = currentBlock.next();

    if (!nextBlock.isValid()) return;

    cursor.beginEditBlock();

    QString currentText = currentBlock.text();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    if (cursor.atBlockStart()) cursor.deleteChar();

    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText("\n" + currentText);

    setTextCursor(cursor);
    cursor.endEditBlock();
}

bool TEditor::eventFilter(QObject* obj, QEvent* event) {
    if (obj == this and event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Return
             or keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                return true;
            }
            curserIndentation();
            return true;
        }
    }
    return QPlainTextEdit::eventFilter(obj, event);
}

void TEditor::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    menu->addSeparator();

    QAction *commentAction = new QAction("تعليق/إلغاء تعليق", this);
    commentAction->setShortcut(QKeySequence("Ctrl+/"));
    connect(commentAction, &QAction::triggered, this, &TEditor::toggleComment);
    menu->addAction(commentAction);

    QAction *duplicateAction = new QAction("تكرار السطر", this);
    duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(duplicateAction, &QAction::triggered, this, &TEditor::duplicateLine);
    menu->addAction(duplicateAction);


    menu->setStyleSheet(
        "QMenu { background-color: #252526; color: #cccccc; border: 1px solid #454545; }"
        "QMenu::item { padding: 5px 20px; background-color: transparent; }"
        "QMenu::item:selected { background-color: #094771; color: #ffffff; }"
        "QMenu::separator { height: 1px; background: #454545; margin: 5px 0; }"
        );

    menu->exec(event->globalPos());

    delete menu;
}

int TEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 30 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}


void TEditor::updateLineNumberAreaWidth() {
    int numsWidth = lineNumberAreaWidth();

    int mapWidth = 0;

    setViewportMargins(mapWidth, 0, numsWidth, 0);
}

inline void TEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void TEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    int numsWidth = lineNumberAreaWidth();


    lineNumberArea->setGeometry(this->width() - numsWidth, cr.top(), numsWidth, cr.height());
}

void TEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::transparent);

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);

            painter.setPen(QColor(200, 200, 200));

            painter.drawText(12, top, lineNumberArea->width(), fontMetrics().height(),
                                     Qt::AlignRight | Qt::AlignVCenter, number);

            bool hasFold = false;
            for (const auto& region : foldRegions) {
                if (region.startBlockNumber == blockNumber) {
                    hasFold = true;
                    bool folded = region.folded;

                    QPolygon arrow;
                    int midY = top + fontMetrics().height() / 2;
                    if (folded) {
                        arrow << QPoint(lineNumberArea->width() - 10, midY - 4)
                        << QPoint(lineNumberArea->width() - 2, midY)
                        << QPoint(lineNumberArea->width() - 10, midY + 4);
                    } else {
                        arrow << QPoint(lineNumberArea->width() - 10, midY - 4)
                        << QPoint(lineNumberArea->width() - 2, midY - 4)
                        << QPoint(lineNumberArea->width() - 6, midY + 4);
                    }

                    painter.setBrush(QColor("#10a8f4"));
                    painter.setPen(Qt::NoPen);
                    painter.drawPolygon(arrow);
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void TEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(23, 24, 36, 240);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void TEditor::updateFoldRegions() {

    QHash<int, bool> previousFoldStates;
    for (const FoldRegion& region : foldRegions) {
        previousFoldStates[region.startBlockNumber] = region.folded;
    }

    foldRegions.clear();
    QStack<int> braceStack;

    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        QString text = block.text();

        QString trimmed = text.trimmed();
        if (trimmed.startsWith("دالة ") || trimmed.startsWith("صنف ")) {
            int start = block.blockNumber();

            int startIndent = 0;
            for (QChar c : text) {
                if (c == '\t') startIndent += 4;
                else if (c == ' ') startIndent += 1;
                else break;
            }

            QTextBlock next = block.next();
            int end = start;

            while (next.isValid()) {
                QString nextText = next.text();
                QString nextTrim = nextText.trimmed();

                if (nextTrim.isEmpty()) {
                    next = next.next();
                    continue;
                }

                int nextIndent = 0;
                for (QChar c : nextText) {
                    if (c == '\t') nextIndent += 4;
                    else if (c == ' ') nextIndent += 1;
                    else break;
                }

                if (nextTrim.startsWith("دالة ") || nextTrim.startsWith("صنف ")) {
                    if (nextIndent <= startIndent)
                        break;
                }

                if (nextIndent <= startIndent)
                    break;

                end = next.blockNumber();
                next = next.next();
            }

            if (end > start) {
                FoldRegion region{};
                region.startBlockNumber = start;
                region.endBlockNumber = end;
                region.folded = false;
                if (previousFoldStates.contains(region.startBlockNumber))
                    region.folded = previousFoldStates[region.startBlockNumber];
                foldRegions.append(region);
            }
        }
        block = block.next();
    }

    if (lineNumberArea)
        lineNumberArea->update();

    for (const FoldRegion& region : foldRegions) {
        QTextBlock block = document()->findBlockByNumber(region.startBlockNumber + 1);
        while (block.isValid() && block.blockNumber() <= region.endBlockNumber) {
            block.setVisible(!region.folded);
            block = block.next();
        }
    }
    document()->markContentsDirty(0, document()->characterCount());
    viewport()->update();
}

void TEditor::toggleFold(int blockNumber) {
    for (FoldRegion &region : foldRegions) {
        if (region.startBlockNumber == blockNumber) {
            region.folded = !region.folded;

            QTextBlock block = document()->findBlockByNumber(region.startBlockNumber + 1);
            while (block.isValid() && block.blockNumber() <= region.endBlockNumber) {
                block.setVisible(!region.folded);
                block = block.next();
            }

            if (!region.folded) {
                for (FoldRegion &subRegion : foldRegions) {
                    if (subRegion.startBlockNumber > region.startBlockNumber &&
                        subRegion.endBlockNumber <= region.endBlockNumber) {
                        QTextBlock subBlock = document()->findBlockByNumber(subRegion.startBlockNumber + 1);
                        bool allVisible = true;
                        while (subBlock.isValid() && subBlock.blockNumber() <= subRegion.endBlockNumber) {
                            if (!subBlock.isVisible()) {
                                allVisible = false;
                                break;
                            }
                            subBlock = subBlock.next();
                        }
                        subRegion.folded = !allVisible;
                    }
                }
            }

            document()->markContentsDirty(0, document()->characterCount());
            viewport()->update();
            break;
        }
    }
}


/* ---------------------------------- Drag and Drop ---------------------------------- */

void TEditor::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.fileName().endsWith(".alif", Qt::CaseInsensitive) or
                url.fileName().endsWith(".aliflib", Qt::CaseInsensitive) or
                url.fileName().endsWith(".txt", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }

    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void TEditor::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void TEditor::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.fileName().endsWith(".alif", Qt::CaseInsensitive) or
                url.fileName().endsWith(".aliflib", Qt::CaseInsensitive) or
                url.fileName().endsWith(".txt", Qt::CaseInsensitive)) {

                QString filePath = url.toLocalFile();
                emit openRequest(filePath);

                event->acceptProposedAction();
                return;
            }
        }
    }

    if (event->mimeData()->hasText()) {
        QTextCursor dropCursor = cursorForPosition(event->position().toPoint());
        int dropPosition = dropCursor.position();

        if (dropPosition >= textCursor().selectionStart()
            and dropPosition <= textCursor().selectionEnd()) {
            event->ignore();
            return;
        }

        QString droppedText = event->mimeData()->text();
        QTextCursor originalCursor = textCursor();

        originalCursor.removeSelectedText();

        if (originalCursor.position() < dropPosition) {
            dropPosition -= droppedText.length();
        }

        dropCursor.setPosition(dropPosition);
        dropCursor.insertText(droppedText);

        event->acceptProposedAction();
        return;
    }

    event->ignore();
}

void TEditor::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}


/* ---------------------------------- Indentation ---------------------------------- */

void TEditor::curserIndentation() {
    QTextCursor cursor = textCursor();
    QString lineText = cursor.block().text();
    int cursorPosInLine = cursor.positionInBlock();
    QString currentIndentation = getCurrentLineIndentation(cursor);

    if (cursorPosInLine > 0) {
        int checkPos = cursorPosInLine - 1;
        while (checkPos >= 0 and lineText.at(checkPos).isSpace()) {
            checkPos--;
        }

        if (checkPos >= 0 and lineText.at(checkPos) == ':') {
            currentIndentation += "\t";
        }
    }

    cursor.beginEditBlock();
    cursor.insertText("\n" + currentIndentation);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

QString TEditor::getCurrentLineIndentation(const QTextCursor &cursor) const {
    QTextBlock block = cursor.block();
    if (!block.isValid()) {
        return QString();
    }

    QString lineText = block.text();
    QString indentation;
    for (const QChar &ch : lineText) {
        if (ch == ' ' or ch == '\t') {
            indentation += ch;
        } else {
            break;
        }
    }
    return indentation;
}




void TEditor::startAutoSave() {
    if (!autoSaveTimer->isActive()) {
        autoSaveTimer->start();
    }
}

void TEditor::stopAutoSave() {
    autoSaveTimer->stop();
}

void TEditor::performAutoSave() {
    QString filePath = this->property("filePath").toString();
    if (filePath.isEmpty() || !this->document()->isModified()) return;

    QString backupPath = filePath + ".~";

    QFile file(backupPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << this->toPlainText();
        file.close();
    }
}

void TEditor::removeBackupFile() {
    QString filePath = this->property("filePath").toString();
    if (filePath.isEmpty()) return;

    QString backupPath = filePath + ".~";
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
    stopAutoSave();
}


void TEditor::updateHighlighterTheme(std::shared_ptr<SyntaxTheme> theme) {
    this->highlighter->setTheme(theme);
}



// --- autocomplete system ---

void TEditor::setupAutoComplete() {
    // set autocomplete system
    model = new CompletionModel(this);
    strategies.push_back(std::make_unique<SnippetStrategy>());
    strategies.push_back(std::make_unique<KeywordStrategy>());
    strategies.push_back(std::make_unique<BuiltinStrategy>());
    strategies.push_back(std::make_unique<DynamicWordStrategy>());

    QCompleter *completer = new QCompleter(this);
    setCompleter(completer);
}

void TEditor::setCompleter(QCompleter *completer) {
    if (c) disconnect(c, nullptr, this, nullptr);
    c = completer;
    if (!c) return;

    c->setWidget(this);
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    c->setModel(model);

    // Custom Rich Popup ---
    TCompletionPopup *popup = new TCompletionPopup;
    c->setPopup(popup); // QCompleter takes ownership

    popup->setItemDelegate(new TModernCompletionDelegate(popup));

    // set dimensions
    popup->setMinimumWidth(350);
    popup->setMinimumHeight(200); // Taller to fit list + footer


    // To this lambda that captures the type:
    connect(c, QOverload<const QString &>::of(&QCompleter::activated),
            this, [this](const QString &completion) {
                // Get the current index from the completer popup
                QModelIndex index = c->popup()->currentIndex();
                if (index.isValid()) {
                    // Get the type from the model
                    CompletionType type = static_cast<CompletionType>(
                        index.data(Qt::UserRole + 2).toInt());
                    // Get the full completion item
                    QString completionText = index.data(Qt::EditRole).toString();
                    insertCompletion(completionText, type);
                } else {
                    // Fallback to just the string without type
                    insertCompletion(completion, CompletionType::DynamicWord);
                }
            });
}

void TEditor::focusOutEvent(QFocusEvent *e) {
    if (c && c->popup()->isVisible()) {
        c->popup()->hide();
    }
    QPlainTextEdit::focusOutEvent(e);
}

void TEditor::keyPressEvent(QKeyEvent *e) {

    // handleing Brackets and Quotes
    if (handleAutoPairing(e)) {
        e->accept();
        return;
    }

    // Handle Navigation for Live Update (Arrow Keys) ---
    if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
        // Let the editor move the cursor first
        QPlainTextEdit::keyPressEvent(e);
        // Then immediately trigger completion to update the list based on the new cursor position
        performCompletion();
        return;
    }

    if (c && c->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return;
        default: break;
        }
    }

    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)) {
        if (!snippetTargets.isEmpty()) {
            if (processSnippetNavigation()) {
                e->accept();
                return;
            }
        }
    }

    if (e->key() == Qt::Key_Tab && !snippetTargets.isEmpty()) {
        if (processSnippetNavigation()) {
            e->accept();
            return;
        }
    }

    bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space);

    QPlainTextEdit::keyPressEvent(e);

    if (!isShortcut && e->text().isEmpty()) return;

    performCompletion();
}

void TEditor::performCompletion() {
    QString textUnder = textUnderCursor();
    // Allow empty text for shortcut (Ctrl+Space) to show all
    if (textUnder.length() < 1) {
        // Optional: Trigger immediately on Ctrl+Space even if empty?
        // For now, keep logic to hide if empty, unless you want "all suggestion" behavior.
        c->popup()->hide();
        return;
    }

    std::vector<CompletionItem> allSuggestions;
    QString fullDoc = toPlainText();

    for (const auto& strategy : strategies) {
        auto res = strategy->getSuggestions(textUnder, fullDoc);
        allSuggestions.insert(allSuggestions.end(), res.begin(), res.end());
    }

    model->updateData(allSuggestions);

    if (allSuggestions.empty()) {
        c->popup()->hide();
        return;
    }

    c->setCompletionPrefix(textUnder);
    QRect cr = cursorRect();

    QPoint widgetPos = this->viewport()->mapTo(this, cr.topRight());
    cr.moveTo(widgetPos);

    // Calculate popup width: Text width + Scrollbar + Padding
    int popupWidth = std::clamp(35 + 150 + c->popup()->verticalScrollBar()->width() + 65, 295, 355);

    // Shift dialog left ---
    cr.moveLeft(cr.right() - popupWidth - 360);

    // set width
    cr.setWidth(popupWidth);

    c->complete(cr);

    // select first item in the popped up list
    QAbstractItemView *popup = c->popup();
    if (popup and popup->model()->rowCount() > 0) {
        popup->setCurrentIndex(popup->model()->index(0, 0));
    }
}

QString TEditor::textUnderCursor() const {
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    return tc.selectedText();
}

void TEditor::insertCompletion(const QString &completion, CompletionType type) {
    if (c->widget() != this) return;
    QTextCursor tc = textCursor();

    // This ensures we replace the whole partial word with the completion.
    tc.select(QTextCursor::WordUnderCursor);

    switch (type) {
    case CompletionType::Builtin:
        insertBuiltinFunction(completion, tc);
        break;
    case CompletionType::Snippet:
        insertSnippet(completion, tc);
        break;
    case CompletionType::Keyword:
        insertWord(completion, tc);
        break;
    case CompletionType::DynamicWord:
    default:
        insertWord(completion, tc);
        break;
    }
}
void TEditor::insertWord(const QString& completion, QTextCursor& tc) {
    tc.insertText(completion);
    setTextCursor(tc);
}
void TEditor::insertBuiltinFunction(const QString& functionName, QTextCursor& tc) {
    // Select everything from cursor to end of current word
    QTextCursor tempCursor = textCursor();
    tempCursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    QString textAfterCursor = tempCursor.selectedText();

    tc.insertText(functionName);
    tc.insertText("()");
    tc.insertText(textAfterCursor);

    tc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, textAfterCursor.length() + 1);

    // Perform the insertion
    setTextCursor(tc);
}
void TEditor::insertSnippet(const QString& snippet, QTextCursor& tc) {
    QString textToInsert = snippet;

    // Calculate indentation
    // Get the full text of the current line to determine indentation
    QTextBlock block = tc.block();
    QString lineText = block.text();
    QString baseIndentation{};
    for (const QChar &ch : lineText) {
        if (ch.isSpace()) baseIndentation.append(ch);
        else break;
    }

    // Apply indentation to multi-line snippets
    if (textToInsert.contains('\n')) {
        QStringList lines = textToInsert.split('\n');
        // Start from index 1 because index 0 is appended to the current line
        // (which already has indentation on the left).
        // Subsequent lines need the base indentation explicitly added.
        for (int i = 1; i < lines.size(); ++i) {
            lines[i] = baseIndentation + lines[i];
        }
        textToInsert = lines.join('\n');
    }

    // Perform the insertion
    tc.insertText(textToInsert);
    setTextCursor(tc);

    // Reset snippet targets
    snippetTargets.clear();

    // Setup snippet navigation based on snippet content
    if (snippet.startsWith("دالة")) {
        QTextCursor finder = textCursor();
        finder.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, textToInsert.length());
        finder = document()->find("اسم", finder);
        if (!finder.isNull()) setTextCursor(finder);
        snippetTargets << "معاملات" << "مرر";
    }
    else if (snippet.startsWith("صنف")) {
        QTextCursor finder = textCursor();
        finder.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, textToInsert.length());
        finder = document()->find("اسم", finder);
        if (!finder.isNull()) setTextCursor(finder);
        snippetTargets << "مرر";
    }
    else if (snippet.startsWith("اذا")) {
        QTextCursor finder = textCursor();
        finder.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, textToInsert.length());
        finder = document()->find("الشرط", finder);
        if (!finder.isNull()) setTextCursor(finder);
        snippetTargets << "مرر";
    }
    else if (snippet.startsWith("لكل")) {
        QTextCursor finder = textCursor();
        finder.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, textToInsert.length());
        finder = document()->find("عنصر", finder);
        if (!finder.isNull()) setTextCursor(finder);
        snippetTargets << "العناصر" << "مرر";
    }
    else if (snippet.startsWith("بينما")) {
        QTextCursor finder = textCursor();
        finder.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, textToInsert.length());
        finder = document()->find("الشرط", finder);
        if (!finder.isNull()) setTextCursor(finder);
        snippetTargets << "مرر";
    }
    else if (snippet.startsWith("حاول")) {
        QTextCursor finder = textCursor();
        finder.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, textToInsert.length());
        finder = document()->find("مرر", finder);
        if (!finder.isNull()) setTextCursor(finder);
        snippetTargets << "مرر";
    }
    else if (snippet.startsWith("خطية")) {
        QTextCursor finder = textCursor();
        finder.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, textToInsert.length());
        finder = document()->find("معاملات", finder);
        if (!finder.isNull()) setTextCursor(finder);
        snippetTargets << "مرر";
    }

}


bool TEditor::processSnippetNavigation() {
    if (snippetTargets.isEmpty()) return false;
    QString nextTarget = snippetTargets.first();
    QTextCursor tc = textCursor();
    QTextCursor found = document()->find(nextTarget, tc);
    if (!found.isNull()) {
        setTextCursor(found);
        snippetTargets.removeFirst();
        return true;
    }
    snippetTargets.clear();
    return false;
}

bool TEditor::handleAutoPairing(QKeyEvent* e) {
    QString text = e->text();

    if (!text.isEmpty()) {
        QChar typedChar = text.at(0);

        // Handle opening brackets
        if (typedChar == '(' || typedChar == '[' || typedChar == '{') {
            QChar closingBracket;
            if (typedChar == '(') closingBracket = ')';
            else if (typedChar == '[') closingBracket = ']';
            else closingBracket = '}';

            return handleBracketCompletion(typedChar, closingBracket);
        }
        // Handle quotes
        else if (typedChar == '\'' || typedChar == '"' || typedChar == '`') {
                return handleQuoteCompletion(typedChar);
        }
        // Handle closing brackets (skip over existing ones)
        else if (typedChar == ')' || typedChar == ']' || typedChar == '}' ||
                 typedChar == '\'' || typedChar == '"' || typedChar == '`') {
            return handleBracketSkip(typedChar);
        }
    }

    return false;
}

bool TEditor::handleBracketCompletion(QChar openingBracket, QChar closingBracket) {
    QTextCursor cursor = textCursor();

    // Check if there's a selection
    if (cursor.hasSelection()) {
        // Wrap selection with brackets
        QString selectedText = cursor.selectedText();
        cursor.insertText(openingBracket + selectedText + closingBracket);

        // Move cursor after the opening bracket to select the original text
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, selectedText.length() + 1);
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, selectedText.length());
        setTextCursor(cursor);
    } else {
        // Insert both brackets and place cursor between them
        cursor.insertText(QString(openingBracket) + closingBracket);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 1);
        setTextCursor(cursor);
    }

    return true;
}

bool TEditor::handleQuoteCompletion(QChar quoteChar) {
    QTextCursor cursor = textCursor();
    QTextDocument *doc = document();

    // Get the character at cursor position
    int pos = cursor.position();

    // Check if there's a selection
    if (cursor.hasSelection()) {
        // Wrap selection with quotes
        QString selectedText = cursor.selectedText();
        cursor.insertText(quoteChar + selectedText + quoteChar);

        // Move cursor after the opening quote to select the original text
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, selectedText.length() + 1);
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, selectedText.length());
        setTextCursor(cursor);
        return true;
    }

    // Check if next character is the same quote (should skip)
    QChar nextChar;
    if (pos < doc->characterCount() - 1) {
        nextChar = doc->characterAt(pos);
        if (nextChar == quoteChar) {
            // Just move cursor over the existing quote
            cursor.movePosition(QTextCursor::Left);
            setTextCursor(cursor);
            return true;
        }
    }

    // Check if we're inside a word (for smart quotes)
    bool insideWord = false;
    if (pos > 0) {
        QChar prevChar = doc->characterAt(pos - 1);
        insideWord = prevChar.isLetterOrNumber() || prevChar == '_';
    }

    // Insert the quote pair
    cursor.insertText(QString(quoteChar) + quoteChar);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 1);
    setTextCursor(cursor);

    return true;
}

bool TEditor::handleBracketSkip(QChar typedChar) {
    QTextCursor cursor = textCursor();
    QTextDocument *doc = document();
    int pos = cursor.position();

    // Check if the next character matches the typed closing bracket/quote
    if (pos < doc->characterCount() - 1) {
        QChar nextChar = doc->characterAt(pos);
        if (nextChar == typedChar) {
            // Just move the cursor over the existing bracket/quote
            cursor.movePosition(QTextCursor::Left);
            setTextCursor(cursor);
            return true;
        }
    }

    return false;
}
