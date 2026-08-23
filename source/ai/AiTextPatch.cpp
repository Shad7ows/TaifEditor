#include "AiTextPatch.h"

#include <algorithm>

namespace {

QString normalizeNewlines(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

QStringList splitLines(const QString& text)
{
    return normalizeNewlines(text).split(QLatin1Char('\n'), Qt::KeepEmptyParts);
}

bool parseEdit(const QJsonObject& object, AiAnchoredLineEdit* const edit, QString* const error)
{
    const QJsonValue startValue = object.value(QStringLiteral("start_line"));
    const QJsonValue endValue = object.value(QStringLiteral("end_line"));
    const QJsonValue expectedValue = object.value(QStringLiteral("expected_text"));
    const QJsonValue replacementValue = object.value(QStringLiteral("replacement"));
    if (!startValue.isDouble() || !endValue.isDouble() || !expectedValue.isString() || !replacementValue.isString()) {
        *error = QStringLiteral("كل تعديل محدد يحتاج start_line وend_line وexpected_text وreplacement.");
        return false;
    }
    edit->startLine = startValue.toInt();
    edit->endLine = endValue.toInt();
    edit->expectedText = normalizeNewlines(expectedValue.toString());
    edit->replacementText = normalizeNewlines(replacementValue.toString());
    return true;
}

} // namespace

AiTextPatchResult AiTextPatch::applyAnchoredLineEdits(const QString& sourceText, const QJsonArray& edits,
                                                       const int maximumEditCount, const int maximumAffectedLines)
{
    AiTextPatchResult result;
    if (edits.isEmpty() || edits.size() > maximumEditCount) {
        result.error = QStringLiteral("عدد تعديلات الأسطر غير صالح أو يتجاوز الحد الآمن.");
        return result;
    }

    const bool sourceUsesCrLf = sourceText.contains(QStringLiteral("\r\n"));
    const QString normalizedSource = normalizeNewlines(sourceText);
    const bool sourceHasFinalNewline = normalizedSource.endsWith(QLatin1Char('\n'));
    QStringList sourceLines = splitLines(normalizedSource);
    if (!sourceLines.isEmpty() && sourceLines.constLast().isEmpty() && normalizedSource.endsWith(QLatin1Char('\n'))) {
        sourceLines.removeLast();
    }
    const int sourceLineCount = sourceLines.size();
    int previousEndLine = 0;
    int affectedLines = 0;

    for (const QJsonValue& value : edits) {
        if (!value.isObject()) {
            result.error = QStringLiteral("صيغة تعديل الأسطر غير صالحة.");
            return result;
        }
        AiAnchoredLineEdit edit;
        if (!parseEdit(value.toObject(), &edit, &result.error)) return result;
        const bool insertion = edit.endLine == edit.startLine - 1;
        if (edit.startLine < 1 || edit.startLine > sourceLineCount + 1
            || (!insertion && (edit.endLine < edit.startLine || edit.endLine > sourceLineCount))) {
            result.error = QStringLiteral("نطاق الأسطر المقترح خارج حدود الملف.");
            return result;
        }
        if (edit.startLine <= previousEndLine) {
            result.error = QStringLiteral("يجب أن تكون تعديلات الأسطر مرتبة وغير متداخلة.");
            return result;
        }
        const int rangeLength = insertion ? 0 : edit.endLine - edit.startLine + 1;
        affectedLines += rangeLength;
        if (affectedLines > maximumAffectedLines) {
            result.error = QStringLiteral("يتجاوز اقتراح التعديل حد الأسطر الآمن.");
            return result;
        }
        const QString actualText = insertion ? QString()
            : sourceLines.mid(edit.startLine - 1, rangeLength).join(QLatin1Char('\n'));
        if (actualText != edit.expectedText) {
            result.error = QStringLiteral("لا يطابق النص المتوقع الأسطر الحالية؛ أعد قراءة المنطقة قبل التعديل.");
            return result;
        }
        previousEndLine = insertion ? edit.startLine - 1 : edit.endLine;
        result.edits.append(edit);
    }

    if (sourceLineCount > 0 && affectedLines >= sourceLineCount) {
        result.error = QStringLiteral("يرفض محرر الأسطر استبدال الملف كاملاً؛ اقترح مناطق أصغر.");
        return result;
    }

    for (auto iterator = result.edits.crbegin(); iterator != result.edits.crend(); ++iterator) {
        const int first = iterator->startLine - 1;
        const int count = iterator->endLine == iterator->startLine - 1 ? 0
            : iterator->endLine - iterator->startLine + 1;
        QStringList replacementLines = splitLines(iterator->replacementText);
        if (!replacementLines.isEmpty() && replacementLines.constLast().isEmpty()
            && iterator->replacementText.endsWith(QLatin1Char('\n'))) {
            replacementLines.removeLast();
        }
        sourceLines = sourceLines.mid(0, first) + replacementLines + sourceLines.mid(first + count);
    }

    result.text = sourceLines.join(QLatin1Char('\n'));
    if (sourceHasFinalNewline) {
        result.text.append(QLatin1Char('\n'));
    }
    if (sourceUsesCrLf) {
        result.text.replace(QStringLiteral("\n"), QStringLiteral("\r\n"));
    }
    result.succeeded = true;
    return result;
}
