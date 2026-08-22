#pragma once

#include <QTextCursor>
#include <QVector>

class QTextDocument;

/** Maintains secondary QTextCursor values and applies one normalized document transaction per multi-edit. */
class MultiCursorController final {
public:
    static constexpr int MaximumSecondaryCursors = 500;

    explicit MultiCursorController(QTextDocument* document = nullptr);

    void setDocument(QTextDocument* document);
    [[nodiscard]] int secondaryCursorCount() const;
    [[nodiscard]] const QVector<QTextCursor>& secondaryCursors() const;
    [[nodiscard]] bool isActive() const;
    void clear();

    [[nodiscard]] bool toggleCursorAt(const QTextCursor& cursor, const QTextCursor& primaryCursor);
    [[nodiscard]] bool addVerticalCursor(const QTextCursor& primaryCursor, int blockDelta);
    [[nodiscard]] bool selectNextOccurrence(const QTextCursor& primaryCursor);
    [[nodiscard]] bool selectAllOccurrences(const QTextCursor& primaryCursor);

    [[nodiscard]] QTextCursor insertText(const QTextCursor& primaryCursor, const QString& text);
    [[nodiscard]] QTextCursor insertTexts(const QTextCursor& primaryCursor, const QVector<QString>& texts);
    [[nodiscard]] QTextCursor insertNewlinesWithIndentation(const QTextCursor& primaryCursor);
    [[nodiscard]] QTextCursor backspace(const QTextCursor& primaryCursor);
    [[nodiscard]] QTextCursor deleteForward(const QTextCursor& primaryCursor);

private:
    struct Edit final {
        int begin = 0;
        int end = 0;
        QString text;
        bool primary = false;
    };

    [[nodiscard]] QVector<QTextCursor> cursorsForEdit(const QTextCursor& primaryCursor) const;
    [[nodiscard]] QTextCursor applyEdits(const QTextCursor& primaryCursor, QVector<Edit> edits);
    void normalizeSecondary(const QTextCursor& primaryCursor);
    [[nodiscard]] bool containsEquivalent(const QTextCursor& cursor) const;

    QTextDocument* m_document = nullptr;
    QVector<QTextCursor> m_secondaryCursors;
};
