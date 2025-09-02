#pragma once

#include <QString>
#include <QSet>

enum class TokenType {
    Keyword,
    Keyword1,
    Keyword2,
    Identifier,
    Number,
    Comment,
    String,
    Operator,
};

class Token {
public:
    TokenType type{};
    int startPos{};
    int len{};
    QString text{};

    Token(TokenType t, int start, int len, QString text) :
        type(t), startPos(start), len(len), text(text) {}
};


class Lexer {
public:
    QVector<Token> tokenize(const QString& text);

private:
    QVector<Token> tokens{};
    int pos = 0;
    int quoteCount = 0;
    int isFString = 0;

    bool isKeyword(const QString& word);
    bool isKeyword1(const QString& word);
    bool isKeyword2(const QString& word);
};
