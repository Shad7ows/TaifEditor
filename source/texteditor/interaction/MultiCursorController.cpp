#include "MultiCursorController.h"

#include <QSet>
#include <QTextBlock>
#include <QTextDocument>

#include <algorithm>

namespace {

bool isValidCursor(const QTextCursor& cursor, const QTextDocument* const document)
{
    return document != nullptr && cursor.document() == document && cursor.position() >= 0
        && cursor.position() <= document->characterCount() - 1;
}

bool isEquivalent(const QTextCursor& left, const QTextCursor& right)
{
    return left.anchor() == right.anchor() && left.position() == right.position();
}

bool overlapsOrTouchesPoint(const QTextCursor& left, const QTextCursor& right)
{
    const int leftBegin = left.selectionStart();
    const int leftEnd = left.selectionEnd();
    const int rightBegin = right.selectionStart();
    const int rightEnd = right.selectionEnd();

    if (!left.hasSelection() && !right.hasSelection()) {
        return leftBegin == rightBegin;
    }
    if (!left.hasSelection()) {
        return leftBegin >= rightBegin && leftBegin <= rightEnd;
    }
    if (!right.hasSelection()) {
        return rightBegin >= leftBegin && rightBegin <= leftEnd;
    }
    return leftBegin < rightEnd && rightBegin < leftEnd;
}

bool cursorOrder(const QTextCursor& left, const QTextCursor& right)
{
    if (left.selectionStart() != right.selectionStart()) {
        return left.selectionStart() < right.selectionStart();
    }
    if (left.selectionEnd() != right.selectionEnd()) {
        return left.selectionEnd() < right.selectionEnd();
    }
    if (left.anchor() != right.anchor()) {
        return left.anchor() < right.anchor();
    }
    return left.position() < right.position();
}

} // namespace

MultiCursorController::MultiCursorController(QTextDocument* const document)
    : m_document(document)
{
}

void MultiCursorController::setDocument(QTextDocument* const document)
{
    m_document = document;
    clear();
}

int MultiCursorController::secondaryCursorCount() const
{
    return m_secondaryCursors.size();
}

const QVector<QTextCursor>& MultiCursorController::secondaryCursors() const
{
    return m_secondaryCursors;
}

bool MultiCursorController::isActive() const
{
    return !m_secondaryCursors.isEmpty();
}

void MultiCursorController::clear()
{
    m_secondaryCursors.clear();
}

bool MultiCursorController::toggleCursorAt(const QTextCursor& cursor, const QTextCursor& primaryCursor)
{
    if (!isValidCursor(cursor, m_document) || !isValidCursor(primaryCursor, m_document)) {
        return false;
    }

    QTextCursor point = cursor;
    point.clearSelection();
    if (overlapsOrTouchesPoint(point, primaryCursor)) {
        return false;
    }

    for (int index = 0; index < m_secondaryCursors.size(); ++index) {
        const QTextCursor& existing = m_secondaryCursors.at(index);
        if (isEquivalent(existing, point)) {
            m_secondaryCursors.removeAt(index);
            return true;
        }
    }
    if (m_secondaryCursors.size() >= MaximumSecondaryCursors) {
        return false;
    }

    m_secondaryCursors.append(point);
    normalizeSecondary(primaryCursor);
    return true;
}

bool MultiCursorController::addVerticalCursor(const QTextCursor& primaryCursor, const int blockDelta)
{
    if (!isValidCursor(primaryCursor, m_document) || blockDelta == 0) {
        return false;
    }

    const QTextBlock source = primaryCursor.block();
    const int targetBlockNumber = source.blockNumber() + blockDelta;
    if (targetBlockNumber < 0) {
        return false;
    }

    const QTextBlock target = m_document->findBlockByNumber(targetBlockNumber);
    if (!target.isValid() || !target.isVisible()) {
        return false;
    }

    QTextCursor candidate(target);
    const int targetColumn = qMin(primaryCursor.positionInBlock(), qMax(0, target.length() - 1));
    candidate.setPosition(target.position() + targetColumn);
    return toggleCursorAt(candidate, primaryCursor);
}

bool MultiCursorController::selectNextOccurrence(const QTextCursor& primaryCursor)
{
    if (!isValidCursor(primaryCursor, m_document) || !primaryCursor.hasSelection()) {
        return false;
    }

    const QString needle = primaryCursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    if (needle.isEmpty() || needle.contains(QLatin1Char('\n'))
        || m_secondaryCursors.size() >= MaximumSecondaryCursors) {
        return false;
    }

    const QVector<QTextCursor> active = cursorsForEdit(primaryCursor);
    QSet<int> inspectedStarts;
    // Continue after the furthest active match, not merely after the primary
    // selection. Repeated Ctrl+Alt+D therefore grows the selection set in
    // document order instead of rediscovering the first secondary match.
    int searchPosition = primaryCursor.selectionEnd();
    for (const QTextCursor& existing : active) {
        searchPosition = qMax(searchPosition, existing.selectionEnd());
    }
    bool wrapped = false;
    while (true) {
        const QTextCursor found = m_document->find(needle, searchPosition);
        if (found.isNull()) {
            if (wrapped) {
                return false;
            }
            wrapped = true;
            searchPosition = 0;
            continue;
        }
        if (inspectedStarts.contains(found.selectionStart())) {
            return false;
        }
        inspectedStarts.insert(found.selectionStart());

        bool conflicts = false;
        for (const QTextCursor& existing : active) {
            if (overlapsOrTouchesPoint(found, existing)) {
                conflicts = true;
                break;
            }
        }
        if (!conflicts) {
            m_secondaryCursors.append(found);
            normalizeSecondary(primaryCursor);
            return true;
        }

        searchPosition = found.selectionEnd();
        if (searchPosition >= m_document->characterCount() - 1 && !wrapped) {
            wrapped = true;
            searchPosition = 0;
        }
    }
}

bool MultiCursorController::selectAllOccurrences(const QTextCursor& primaryCursor)
{
    if (!isValidCursor(primaryCursor, m_document) || !primaryCursor.hasSelection()) {
        return false;
    }

    const QString needle = primaryCursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    if (needle.isEmpty() || needle.contains(QLatin1Char('\n'))) {
        return false;
    }

    QVector<QTextCursor> matches;
    int searchPosition = 0;
    while (true) {
        const QTextCursor found = m_document->find(needle, searchPosition);
        if (found.isNull()) {
            break;
        }
        if (!overlapsOrTouchesPoint(found, primaryCursor)) {
            matches.append(found);
            if (matches.size() >= MaximumSecondaryCursors) {
                break;
            }
        }
        searchPosition = found.selectionEnd();
    }

    m_secondaryCursors = std::move(matches);
    normalizeSecondary(primaryCursor);
    return isActive();
}

QVector<QTextCursor> MultiCursorController::cursorsForEdit(const QTextCursor& primaryCursor) const
{
    QVector<QTextCursor> cursors;
    if (!isValidCursor(primaryCursor, m_document)) {
        return cursors;
    }

    // The primary cursor always remains the first and authoritative edit target.
    cursors.append(primaryCursor);

    QVector<QTextCursor> secondaries;
    secondaries.reserve(m_secondaryCursors.size());
    for (const QTextCursor& cursor : m_secondaryCursors) {
        if (isValidCursor(cursor, m_document)) {
            secondaries.append(cursor);
        }
    }
    std::sort(secondaries.begin(), secondaries.end(), cursorOrder);

    for (const QTextCursor& candidate : secondaries) {
        bool conflicts = false;
        for (const QTextCursor& retained : cursors) {
            if (overlapsOrTouchesPoint(candidate, retained)) {
                conflicts = true;
                break;
            }
        }
        if (!conflicts) {
            cursors.append(candidate);
        }
    }
    return cursors;
}

QTextCursor MultiCursorController::insertText(const QTextCursor& primaryCursor, const QString& text)
{
    if (!isActive() || text.isEmpty()) {
        return primaryCursor;
    }

    const QVector<QTextCursor> cursors = cursorsForEdit(primaryCursor);
    QVector<QString> texts;
    texts.fill(text, cursors.size());
    return insertTexts(primaryCursor, texts);
}

QTextCursor MultiCursorController::insertTexts(const QTextCursor& primaryCursor, const QVector<QString>& texts)
{
    if (!isActive()) {
        return primaryCursor;
    }

    const QVector<QTextCursor> cursors = cursorsForEdit(primaryCursor);
    if (cursors.isEmpty() || cursors.size() != texts.size()) {
        return primaryCursor;
    }

    QVector<Edit> edits;
    edits.reserve(cursors.size());
    for (int index = 0; index < cursors.size(); ++index) {
        const QTextCursor& cursor = cursors.at(index);
        edits.append({cursor.selectionStart(), cursor.selectionEnd(), texts.at(index), index == 0});
    }
    return applyEdits(primaryCursor, std::move(edits));
}

QTextCursor MultiCursorController::insertNewlinesWithIndentation(const QTextCursor& primaryCursor)
{
    if (!isActive() || m_document == nullptr) {
        return primaryCursor;
    }

    const QVector<QTextCursor> cursors = cursorsForEdit(primaryCursor);
    QVector<QString> texts;
    texts.reserve(cursors.size());
    for (const QTextCursor& cursor : cursors) {
        const QString lineText = cursor.block().text();
        const int positionInLine = cursor.positionInBlock();
        QString indentation;
        for (const QChar character : lineText) {
            if (character == QLatin1Char(' ') || character == QLatin1Char('\t')) {
                indentation.append(character);
            } else {
                break;
            }
        }

        int checkPosition = positionInLine - 1;
        while (checkPosition >= 0 && lineText.at(checkPosition).isSpace()) {
            --checkPosition;
        }
        if (checkPosition >= 0 && lineText.at(checkPosition) == QLatin1Char(':')) {
            indentation.append(QLatin1Char('\t'));
        }
        texts.append(QLatin1Char('\n') + indentation);
    }
    return insertTexts(primaryCursor, texts);
}

QTextCursor MultiCursorController::backspace(const QTextCursor& primaryCursor)
{
    if (!isActive()) {
        return primaryCursor;
    }

    QVector<Edit> edits;
    const QVector<QTextCursor> cursors = cursorsForEdit(primaryCursor);
    for (int index = 0; index < cursors.size(); ++index) {
        const QTextCursor& cursor = cursors.at(index);
        const int begin = cursor.hasSelection() ? cursor.selectionStart() : qMax(0, cursor.position() - 1);
        const int end = cursor.hasSelection() ? cursor.selectionEnd() : cursor.position();
        if (begin != end) {
            edits.append({begin, end, {}, index == 0});
        }
    }
    return edits.isEmpty() ? primaryCursor : applyEdits(primaryCursor, std::move(edits));
}

QTextCursor MultiCursorController::deleteForward(const QTextCursor& primaryCursor)
{
    if (!isActive() || m_document == nullptr) {
        return primaryCursor;
    }

    const int documentLength = qMax(0, m_document->characterCount() - 1);
    QVector<Edit> edits;
    const QVector<QTextCursor> cursors = cursorsForEdit(primaryCursor);
    for (int index = 0; index < cursors.size(); ++index) {
        const QTextCursor& cursor = cursors.at(index);
        const int begin = cursor.hasSelection() ? cursor.selectionStart() : cursor.position();
        const int end = cursor.hasSelection() ? cursor.selectionEnd() : qMin(documentLength, cursor.position() + 1);
        if (begin != end) {
            edits.append({begin, end, {}, index == 0});
        }
    }
    return edits.isEmpty() ? primaryCursor : applyEdits(primaryCursor, std::move(edits));
}

QTextCursor MultiCursorController::applyEdits(const QTextCursor& primaryCursor, QVector<Edit> edits)
{
    if (m_document == nullptr || edits.isEmpty()) {
        return primaryCursor;
    }

    std::sort(edits.begin(), edits.end(), [](const Edit& left, const Edit& right) {
        if (left.begin != right.begin) {
            return left.begin > right.begin;
        }
        if (left.end != right.end) {
            return left.end > right.end;
        }
        return left.primary;
    });

    QTextCursor transaction(m_document);
    transaction.beginEditBlock();
    QTextCursor finalPrimary = primaryCursor;
    for (const Edit& edit : edits) {
        QTextCursor worker(m_document);
        worker.setPosition(edit.begin);
        worker.setPosition(edit.end, QTextCursor::KeepAnchor);
        worker.insertText(edit.text);
        if (edit.primary) {
            finalPrimary = worker;
        }
    }
    transaction.endEditBlock();

    normalizeSecondary(finalPrimary);
    return finalPrimary;
}

void MultiCursorController::normalizeSecondary(const QTextCursor& primaryCursor)
{
    const QVector<QTextCursor> normalized = cursorsForEdit(primaryCursor);
    m_secondaryCursors.clear();
    for (int index = 1; index < normalized.size(); ++index) {
        m_secondaryCursors.append(normalized.at(index));
    }
}

bool MultiCursorController::containsEquivalent(const QTextCursor& cursor) const
{
    for (const QTextCursor& existing : m_secondaryCursors) {
        if (isEquivalent(existing, cursor)) {
            return true;
        }
    }
    return false;
}
