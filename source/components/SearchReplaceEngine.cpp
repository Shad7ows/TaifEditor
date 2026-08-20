#include "SearchReplaceEngine.h"

#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>

namespace {

bool isWordCharacter(const QChar character)
{
    return character.isLetterOrNumber() || character == u'_';
}

bool isWholeWordMatch(const QString& text, const int start, const int length)
{
    const int end = start + length;
    const bool leftIsWord = start > 0 && isWordCharacter(text.at(start - 1));
    const bool rightIsWord = end < text.size() && isWordCharacter(text.at(end));
    return !leftIsWord && !rightIsWord;
}

QRegularExpression expressionFor(const SearchReplaceEngine::Query& query)
{
    return QRegularExpression(
        query.pattern,
        query.caseSensitive ? QRegularExpression::NoPatternOption
                            : QRegularExpression::CaseInsensitiveOption);
}

} // namespace

namespace SearchReplaceEngine {

bool isValid(const Query& query)
{
    return !query.useRegex || expressionFor(query).isValid();
}

QList<MatchRange> collectMatches(const QString& text, const Query& query)
{
    QList<MatchRange> matches;
    if (text.isEmpty() || query.pattern.isEmpty() || !isValid(query)) {
        return matches;
    }

    if (query.useRegex) {
        QRegularExpressionMatchIterator iterator = expressionFor(query).globalMatch(text);
        while (iterator.hasNext()) {
            const QRegularExpressionMatch match = iterator.next();
            const int start = match.capturedStart();
            const int length = match.capturedLength();
            if (length > 0 && (!query.wholeWord || isWholeWordMatch(text, start, length))) {
                matches.append({start, length});
            }
        }
        return matches;
    }

    const Qt::CaseSensitivity sensitivity = query.caseSensitive
        ? Qt::CaseSensitive
        : Qt::CaseInsensitive;
    for (int position = 0;
         (position = text.indexOf(query.pattern, position, sensitivity)) >= 0;
         position += qMax(1, query.pattern.size())) {
        if (!query.wholeWord || isWholeWordMatch(text, position, query.pattern.size())) {
            matches.append({position, query.pattern.size()});
        }
    }
    return matches;
}

QString replacementForMatch(const QString& matchedText, const Query& query)
{
    if (!query.useRegex) {
        return query.replacement;
    }

    const QRegularExpression expression = expressionFor(query);
    if (!expression.isValid() || !expression.match(matchedText).hasMatch()) {
        return query.replacement;
    }

    QString expanded = matchedText;
    expanded.replace(expression, query.replacement);
    return expanded;
}

void replaceAll(QTextDocument* const document,
                const QString& originalText,
                const QList<MatchRange>& matches,
                const Query& query)
{
    if (document == nullptr || matches.isEmpty()) {
        return;
    }

    QTextCursor transaction(document);
    transaction.beginEditBlock();
    for (int index = matches.size() - 1; index >= 0; --index) {
        const MatchRange& match = matches.at(index);
        transaction.setPosition(match.first);
        transaction.setPosition(match.first + match.second, QTextCursor::KeepAnchor);
        transaction.insertText(replacementForMatch(
            originalText.mid(match.first, match.second), query));
    }
    transaction.endEditBlock();
}

} // namespace SearchReplaceEngine
