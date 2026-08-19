#pragma once

#include <QString>
#include <QVector>
#include <QStringList>

enum CompletionType {
    Keyword,
    Snippet,
    Builtin,
    DynamicWord,
    SemanticSymbol
};

struct CompletionItem {
    QString label;
    QString completion;
    QString description; // New field for the UI
    CompletionType type;
};

// Abstract Strategy Interface
class ICompletionStrategy {
public:
    virtual ~ICompletionStrategy() = default;
    virtual QVector<CompletionItem> getSuggestions(const QString &prefix, const QString &fullText) = 0;
};

// --- Concrete Strategies ---

class KeywordStrategy : public ICompletionStrategy {
    QStringList keywords{};
public:
    KeywordStrategy();
    QVector<CompletionItem> getSuggestions(const QString &prefix, const QString &text) override;
};

class BuiltinStrategy : public ICompletionStrategy {
    QStringList builtins{};
public:
    BuiltinStrategy();
    QVector<CompletionItem> getSuggestions(const QString &prefix, const QString &text) override;
};

class SnippetStrategy : public ICompletionStrategy {
public:
    QVector<CompletionItem> getSuggestions(const QString &prefix, const QString &text) override;
};

class DynamicWordStrategy : public ICompletionStrategy {
public:
    QVector<CompletionItem> getSuggestions(const QString &prefix, const QString &fullText) override;
};

