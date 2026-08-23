#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

/**
 * Applies a small ordered set of model-proposed whole-line replacements to a
 * text snapshot. Every edit is anchored by its expected existing text so the
 * proposal fails closed if the intended source region differs.
 */
struct AiAnchoredLineEdit final {
    int startLine = 0; // One-based, inclusive.
    int endLine = 0;   // One-based, inclusive. startLine - 1 denotes insertion.
    QString expectedText;
    QString replacementText;
};

struct AiTextPatchResult final {
    bool succeeded = false;
    QString text;
    QString error;
    QVector<AiAnchoredLineEdit> edits;
};

class AiTextPatch final {
public:
    static AiTextPatchResult applyAnchoredLineEdits(const QString& sourceText, const QJsonArray& edits,
                                                    int maximumEditCount = 32, int maximumAffectedLines = 160);
};
