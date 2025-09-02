#pragma once
#include "AlifLexer.h"
#include <QSyntaxHighlighter>



class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    bool isFunctionName(const QString& blockText, int idEndPos);
    QVector<Token> tokens{};
};
