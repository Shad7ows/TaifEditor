#pragma once

#include <QString>
#include <QVector>

/** A single aligned row in an original-versus-proposed text comparison. */
struct AiLineDiffRow final {
    enum class Kind : quint8 {
        Unchanged,
        Removed,
        Added
    };

    Kind kind = Kind::Unchanged;
    int originalLine = -1;
    int proposedLine = -1;
    QString originalText;
    QString proposedText;
};

struct AiLineDiffSummary final {
    int addedLines = 0;
    int removedLines = 0;
    int unchangedLines = 0;
    bool usedWholeDocumentFallback = false;
};

struct AiLineDiffResult final {
    QVector<AiLineDiffRow> rows;
    AiLineDiffSummary summary;
};

/**
 * Produces deterministic line alignment for the staged patch review UI.
 * The implementation falls back to bounded whole-document rows when a full
 * dynamic-programming matrix would exceed its fixed resource budget.
 */
class AiLineDiff final {
public:
    [[nodiscard]] static AiLineDiffResult compare(const QString& original, const QString& proposed);

private:
    AiLineDiff() = delete;
};
