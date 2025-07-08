#include "SPHighlighter.h"


SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
}

void SyntaxHighlighter::highlightBlock(const QString& text) {
    Lexer lexer{};
    QVector<Token> tokens = lexer.tokenize(text);

    for (const auto& token : tokens) {
        QTextCharFormat format;
        switch (token.type) {
        case TokenType::Keyword:
            format.setForeground(QColor(254, 135, 48));
            // format.setForeground(QColor(255, 121, 198));
            break;
        case TokenType::Keyword1:
            format.setForeground(QColor(218, 183, 68));
            // format.setForeground(QColor(222, 49, 99));
            break;
        case TokenType::Keyword2:
            format.setForeground(QColor(121, 129, 230));
            // format.setForeground(QColor(204, 204, 255));
            break;
        case TokenType::Number:
            format.setForeground(QColor(168, 135, 206));
            // format.setForeground(QColor(255, 184, 108));
            break;
        case TokenType::Identifier:
            if (isFunctionName(text, token.startPos + token.len)) {
                format.setForeground(QColor(206, 147, 74));
                // format.setForeground(QColor(139, 233, 253));
            }
            break;
        case TokenType::Comment:
            format.setForeground(QColor(85, 91, 100));
            // format.setForeground(QColor(98, 114, 164));
            break;
        case TokenType::String:
            // format.setForeground(QColor(78, 116, 51));
            // format.setForeground(QColor(80, 250, 123));
            format.setForeground(QColor(0, 175, 4));
            break;
        case TokenType::Operator:
            format.setForeground(QColor(224, 108, 117));
            break;
        default:
            break;
        }
        setFormat(token.startPos, token.len, format);
    }
}



bool SyntaxHighlighter::isFunctionName(const QString& blockText, int idEndPos) {
    if (idEndPos < blockText.length() and blockText[idEndPos] == '(') {
        return true;
    }
    return false;
}
