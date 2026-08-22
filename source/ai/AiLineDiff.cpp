#include "AiLineDiff.h"

#include <algorithm>

namespace {
constexpr qsizetype kMaximumMatrixCells = 1000000;

QStringList splitLines(const QString& text)
{
    return text.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
}

void appendRemoved(AiLineDiffResult* const result, const int originalLine, const QString& text)
{
    result->rows.append({AiLineDiffRow::Kind::Removed, originalLine, -1, text, {}});
    ++result->summary.removedLines;
}

void appendAdded(AiLineDiffResult* const result, const int proposedLine, const QString& text)
{
    result->rows.append({AiLineDiffRow::Kind::Added, -1, proposedLine, {}, text});
    ++result->summary.addedLines;
}

void appendUnchanged(AiLineDiffResult* const result, const int originalLine, const int proposedLine,
                     const QString& text)
{
    result->rows.append({AiLineDiffRow::Kind::Unchanged, originalLine, proposedLine, text, text});
    ++result->summary.unchangedLines;
}
}

AiLineDiffResult AiLineDiff::compare(const QString& original, const QString& proposed)
{
    const QStringList originalLines = splitLines(original);
    const QStringList proposedLines = splitLines(proposed);
    const qsizetype originalCount = originalLines.size();
    const qsizetype proposedCount = proposedLines.size();

    AiLineDiffResult result;
    if (originalCount == proposedCount && originalLines == proposedLines) {
        result.rows.reserve(originalCount);
        for (qsizetype index = 0; index < originalCount; ++index) {
            appendUnchanged(&result, static_cast<int>(index) + 1, static_cast<int>(index) + 1,
                            originalLines.at(index));
        }
        return result;
    }

    if ((originalCount + 1) > 0 && (proposedCount + 1) > kMaximumMatrixCells / (originalCount + 1)) {
        result.summary.usedWholeDocumentFallback = true;
        result.rows.reserve(originalCount + proposedCount);
        for (qsizetype index = 0; index < originalCount; ++index) {
            appendRemoved(&result, static_cast<int>(index) + 1, originalLines.at(index));
        }
        for (qsizetype index = 0; index < proposedCount; ++index) {
            appendAdded(&result, static_cast<int>(index) + 1, proposedLines.at(index));
        }
        return result;
    }

    const qsizetype columns = proposedCount + 1;
    QVector<int> lcs((originalCount + 1) * columns, 0);
    const auto indexFor = [columns](const qsizetype originalIndex, const qsizetype proposedIndex) {
        return originalIndex * columns + proposedIndex;
    };
    for (qsizetype originalIndex = 1; originalIndex <= originalCount; ++originalIndex) {
        for (qsizetype proposedIndex = 1; proposedIndex <= proposedCount; ++proposedIndex) {
            if (originalLines.at(originalIndex - 1) == proposedLines.at(proposedIndex - 1)) {
                lcs[indexFor(originalIndex, proposedIndex)] = lcs[indexFor(originalIndex - 1, proposedIndex - 1)] + 1;
            } else {
                lcs[indexFor(originalIndex, proposedIndex)] = qMax(lcs[indexFor(originalIndex - 1, proposedIndex)],
                                                                    lcs[indexFor(originalIndex, proposedIndex - 1)]);
            }
        }
    }

    QVector<AiLineDiffRow> reversedRows;
    reversedRows.reserve(originalCount + proposedCount);
    qsizetype originalIndex = originalCount;
    qsizetype proposedIndex = proposedCount;
    while (originalIndex > 0 || proposedIndex > 0) {
        if (originalIndex > 0 && proposedIndex > 0
            && originalLines.at(originalIndex - 1) == proposedLines.at(proposedIndex - 1)) {
            reversedRows.append({AiLineDiffRow::Kind::Unchanged, static_cast<int>(originalIndex),
                                 static_cast<int>(proposedIndex), originalLines.at(originalIndex - 1),
                                 proposedLines.at(proposedIndex - 1)});
            --originalIndex;
            --proposedIndex;
        } else if (proposedIndex > 0 && (originalIndex == 0
                                         || lcs[indexFor(originalIndex, proposedIndex - 1)]
                                             >= lcs[indexFor(originalIndex - 1, proposedIndex)])) {
            reversedRows.append({AiLineDiffRow::Kind::Added, -1, static_cast<int>(proposedIndex), {},
                                 proposedLines.at(proposedIndex - 1)});
            --proposedIndex;
        } else {
            reversedRows.append({AiLineDiffRow::Kind::Removed, static_cast<int>(originalIndex), -1,
                                 originalLines.at(originalIndex - 1), {}});
            --originalIndex;
        }
    }

    std::reverse(reversedRows.begin(), reversedRows.end());
    result.rows = std::move(reversedRows);
    for (const AiLineDiffRow& row : result.rows) {
        switch (row.kind) {
        case AiLineDiffRow::Kind::Unchanged: ++result.summary.unchangedLines; break;
        case AiLineDiffRow::Kind::Removed: ++result.summary.removedLines; break;
        case AiLineDiffRow::Kind::Added: ++result.summary.addedLines; break;
        }
    }
    return result;
}
