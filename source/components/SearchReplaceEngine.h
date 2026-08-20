#pragma once

#include <QList>
#include <QPair>
#include <QString>

class QTextDocument;

namespace SearchReplaceEngine {

using MatchRange = QPair<int, int>;

struct Query final {
    QString pattern;
    QString replacement;
    bool caseSensitive = false;
    bool wholeWord = false;
    bool useRegex = false;
};

[[nodiscard]] bool isValid(const Query& query);
[[nodiscard]] QList<MatchRange> collectMatches(const QString& text, const Query& query);
[[nodiscard]] QString replacementForMatch(const QString& matchedText, const Query& query);

/**
 * Replaces the supplied ranges in reverse document order inside one undoable
 * edit block. The ranges must have been collected from originalText with query.
 */
void replaceAll(QTextDocument* document,
                const QString& originalText,
                const QList<MatchRange>& matches,
                const Query& query);

} // namespace SearchReplaceEngine
