#include "TaifLexer.h"

#include <QtTest/QTest>
#include <QtCore/QFile>

namespace {

QVector<TokenKind> mainKinds(const LexResult& result) {
    QVector<TokenKind> kinds;
    for (const Token& token : result.tokens) {
        if (token.channel == TokenChannel::Main) {
            kinds.append(token.kind);
        }
    }
    return kinds;
}

void compareKinds(const LexResult& result, const QVector<TokenKind>& expected) {
    const QVector<TokenKind> actual = mainKinds(result);
    QCOMPARE(actual.size(), expected.size());
    for (qsizetype index = 0; index < actual.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

} // namespace

class TaifLexerTest final : public QObject {
    Q_OBJECT

private slots:
    void emptyFileProducesEndOfFile();
    void statementSeparatorsAndKeywordAliases();
    void layoutTriviaAndSourceRanges();
    void literalsAndLongestMatchOperators();
    void formattedStringsHaveStructuredTokens();
    void formattedStringFormatsAreStructured();
    void malformedLiteralReportsDiagnostic();
    void statusCorpusLexesToEndOfFile();
};

void TaifLexerTest::emptyFileProducesEndOfFile() {
    const LexResult result = TaifLexer().lex(QString());

    QCOMPARE(result.diagnostics.size(), 0);
    QCOMPARE(result.tokens.size(), 1);
    QCOMPARE(result.tokens.constFirst().kind, TokenKind::EndOfFile);
    QCOMPARE(result.tokens.constFirst().range.begin.offset, qsizetype(0));
    QCOMPARE(result.tokens.constFirst().range.end.offset, qsizetype(0));
}

void TaifLexerTest::statementSeparatorsAndKeywordAliases() {
    const LexResult statement = TaifLexer().lex(QStringLiteral("س = 5؛ ص = 9\n"));
    QCOMPARE(statement.diagnostics.size(), 0);
    compareKinds(statement, {
        TokenKind::Identifier, TokenKind::Equal, TokenKind::IntegerLiteral,
        TokenKind::ArabicSemicolon, TokenKind::Identifier, TokenKind::Equal,
        TokenKind::IntegerLiteral, TokenKind::Newline, TokenKind::EndOfFile
    });

    const LexResult aliases = TaifLexer().lex(QStringLiteral("إذا صح و ليس عدم\n"));
    QCOMPARE(aliases.diagnostics.size(), 0);
    compareKinds(aliases, {
        TokenKind::KwIf, TokenKind::KwTrue, TokenKind::KwAnd,
        TokenKind::KwNot, TokenKind::KwNull, TokenKind::Newline,
        TokenKind::EndOfFile
    });
}

void TaifLexerTest::layoutTriviaAndSourceRanges() {
    const QString source = QStringLiteral("# تعليق\nاذا س:\n\tاطبع(س)\nوالا:\n\tاطبع(خطأ)\n");
    const LexResult result = TaifLexer().lex(source);

    QCOMPARE(result.diagnostics.size(), 0);
    compareKinds(result, {
        TokenKind::Newline,
        TokenKind::KwIf, TokenKind::Identifier, TokenKind::Colon, TokenKind::Newline,
        TokenKind::Indent, TokenKind::Identifier, TokenKind::LParen,
        TokenKind::Identifier, TokenKind::RParen, TokenKind::Newline,
        TokenKind::Dedent, TokenKind::KwElse, TokenKind::Colon, TokenKind::Newline,
        TokenKind::Indent, TokenKind::Identifier, TokenKind::LParen,
        TokenKind::KwFalse, TokenKind::RParen, TokenKind::Newline,
        TokenKind::Dedent, TokenKind::EndOfFile
    });

    QVERIFY(!result.tokens.isEmpty());
    for (const Token& token : result.tokens) {
        QVERIFY(token.range.begin.offset <= token.range.end.offset);
        QVERIFY(token.range.begin.line >= 1);
        QVERIFY(token.range.begin.column >= 1);
    }

    bool foundComment = false;
    bool foundIndentTrivia = false;
    for (const Token& token : result.tokens) {
        if (token.kind == TokenKind::Comment && token.channel == TokenChannel::Trivia) {
            foundComment = true;
        }
        if (token.kind == TokenKind::Whitespace && token.channel == TokenChannel::Trivia
            && token.lexeme == QStringLiteral("\t")) {
            foundIndentTrivia = true;
        }
    }
    QVERIFY(foundComment);
    QVERIFY(foundIndentTrivia);
}

void TaifLexerTest::literalsAndLongestMatchOperators() {
    const LexResult result = TaifLexer().lex(QStringLiteral("0x2A \\*= 3 \\\\= 5 ^ 2\n"));

    QCOMPARE(result.diagnostics.size(), 0);
    compareKinds(result, {
        TokenKind::IntegerLiteral, TokenKind::StarSlashEqual,
        TokenKind::IntegerLiteral, TokenKind::DoubleSlashEqual,
        TokenKind::IntegerLiteral, TokenKind::Power,
        TokenKind::IntegerLiteral, TokenKind::Newline, TokenKind::EndOfFile
    });
}

void TaifLexerTest::formattedStringsHaveStructuredTokens() {
    const LexResult result = TaifLexer().lex(QStringLiteral("س = م\"قيمة {س + 1}\"\n"));

    QCOMPARE(result.diagnostics.size(), 0);
    compareKinds(result, {
        TokenKind::Identifier, TokenKind::Equal,
        TokenKind::FStringStart, TokenKind::FStringText,
        TokenKind::InterpolationStart, TokenKind::Identifier,
        TokenKind::Plus, TokenKind::IntegerLiteral,
        TokenKind::InterpolationEnd, TokenKind::FStringEnd,
        TokenKind::Newline, TokenKind::EndOfFile
    });
}

void TaifLexerTest::formattedStringFormatsAreStructured() {
    const LexResult result = TaifLexer().lex(QStringLiteral("م\"{س:.2ف}%\""));

    QCOMPARE(result.diagnostics.size(), 0);
    compareKinds(result, {
        TokenKind::FStringStart, TokenKind::InterpolationStart,
        TokenKind::Identifier, TokenKind::Colon, TokenKind::FStringFormat,
        TokenKind::InterpolationEnd, TokenKind::FStringText, TokenKind::FStringEnd,
        TokenKind::EndOfFile
    });
}

void TaifLexerTest::malformedLiteralReportsDiagnostic() {
    const LexResult result = TaifLexer().lex(QStringLiteral("س = \"غير منته\n"));

    QVERIFY(!result.diagnostics.isEmpty());
    QCOMPARE(result.diagnostics.constFirst().code, QStringLiteral("LEX002"));
    QVERIFY(result.tokens.constLast().kind == TokenKind::EndOfFile);
}

void TaifLexerTest::statusCorpusLexesToEndOfFile() {
    QFile file(QFINDTESTDATA("data/Status.alif"));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    const QString corpus = QString::fromUtf8(file.readAll());
    QVERIFY(!corpus.isEmpty());

    const LexResult result = TaifLexer().lex(corpus);
    QVERIFY(!result.tokens.isEmpty());
    QCOMPARE(result.tokens.constLast().kind, TokenKind::EndOfFile);
    QStringList diagnosticSummary;
    for (const LexDiagnostic& diagnostic : result.diagnostics) {
        diagnosticSummary.append(QStringLiteral("%1 at %2:%3: %4")
            .arg(diagnostic.code)
            .arg(diagnostic.range.begin.line)
            .arg(diagnostic.range.begin.column)
            .arg(diagnostic.message));
    }
    QVERIFY2(result.diagnostics.isEmpty(), qPrintable(diagnosticSummary.join(QStringLiteral(" | "))));

    bool foundFString = false;
    bool foundIndent = false;
    bool foundArabicSemicolon = false;
    for (const Token& token : result.tokens) {
        foundFString = foundFString || token.kind == TokenKind::FStringStart;
        foundIndent = foundIndent || token.kind == TokenKind::Indent;
        foundArabicSemicolon = foundArabicSemicolon || token.kind == TokenKind::ArabicSemicolon;
    }
    QVERIFY(foundFString);
    QVERIFY(foundIndent);
    QVERIFY(foundArabicSemicolon);
}

QTEST_GUILESS_MAIN(TaifLexerTest)
#include "tst_TaifLexer.moc"
