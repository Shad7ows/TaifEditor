#include "TaifParser.h"

#include <QtCore/QFile>
#include <QtTest/QTest>

namespace {

bool hasAstKind(const AstModule& module, const AstNodeKind kind) {
    for (const AstNode& node : module.nodes()) {
        if (node.kind == kind) {
            return true;
        }
    }
    return false;
}

bool hasDiagnostic(const ParseResult& result, const QString& code) {
    for (const ParseDiagnostic& diagnostic : result.parserDiagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

} // namespace

class TaifParserTest final : public QObject {
    Q_OBJECT

private slots:
    void emptySourceCreatesCompleteSnapshots();
    void parserNormalizesMissingEndOfFile();
    void declarationsAndSuitesCreateSemanticNodes();
    void forHeaderTreatsInAsAStructuralDelimiter();
    void expressionsUsePrattPostfixAndPrecedenceParsing();
    void formattedStringCreatesStructuredAst();
    void recoveryPreservesLaterDeclarations();
    void lexerDiagnosticsAreRetained();
    void reparseFallsBackToAnEquivalentFreshSnapshot();
    void statusCorpusProducesAFiniteTree();
};

void TaifParserTest::emptySourceCreatesCompleteSnapshots() {
    const ParseResult result = TaifParser().parse(QString());

    QVERIFY(result.syntaxTree != nullptr);
    QVERIFY(result.ast != nullptr);
    QCOMPARE(result.syntaxTree->root().kind, SyntaxKind::Module);
    QCOMPARE(result.ast->root().kind, AstNodeKind::Module);
    QVERIFY(result.parserDiagnostics.isEmpty());
    QCOMPARE(result.syntaxTree->tokens().constLast().kind, TokenKind::EndOfFile);
}

void TaifParserTest::parserNormalizesMissingEndOfFile() {
    LexResult lexicalResult;
    Token identifier;
    identifier.kind = TokenKind::Identifier;
    identifier.channel = TokenChannel::Main;
    identifier.lexeme = QStringLiteral("س");
    identifier.range = {{0, 1, 1}, {1, 1, 2}};
    lexicalResult.tokens.append(identifier);

    const ParseResult result = TaifParser().parse(QStringLiteral("س"), lexicalResult);
    QVERIFY(result.syntaxTree != nullptr);
    QCOMPARE(result.syntaxTree->tokens().constLast().kind, TokenKind::EndOfFile);
    QVERIFY(result.ast != nullptr);
}

void TaifParserTest::declarationsAndSuitesCreateSemanticNodes() {
    const QString source = QStringLiteral(
        "دالة جمع(س, ص = 1):\n"
        "\tارجع س + ص\n"
        "صنف مثال:\n"
        "\tدالة __تهيئة__(هذا):\n"
        "\t\tهذا.س = 1\n");
    const ParseResult result = TaifParser().parse(source);

    QVERIFY(result.syntaxTree != nullptr);
    QVERIFY(result.ast != nullptr);
    QVERIFY(result.parserDiagnostics.isEmpty());
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::FunctionDeclaration));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::ClassDeclaration));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::Parameter));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::AssignmentStatement));
}

void TaifParserTest::forHeaderTreatsInAsAStructuralDelimiter() {
    const QString source = QStringLiteral(
        "لكل ب في مدى(5):\n"
        "\tاطبع(ب)\n");
    const ParseResult result = TaifParser().parse(source);

    QVERIFY(result.parserDiagnostics.isEmpty());
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::ForStatement));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::CallExpression));
}

void TaifParserTest::expressionsUsePrattPostfixAndPrecedenceParsing() {
    const QString source = QStringLiteral(
        "س = -2 + 3 * 4 ^ 2\n"
        "اطبع(س.قيمة[1:3], اسم = \"نص\", *ض, **ص)\n");
    const ParseResult result = TaifParser().parse(source);

    QVERIFY(result.parserDiagnostics.isEmpty());
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::UnaryExpression));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::BinaryExpression));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::CallExpression));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::MemberExpression));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::SliceExpression));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::KeywordArgument));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::StarArgument));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::DoubleStarArgument));
}

void TaifParserTest::formattedStringCreatesStructuredAst() {
    const ParseResult result = TaifParser().parse(QStringLiteral(
        "س = م\"القيمة {س + 1:.2ف}\"\n"));

    QVERIFY(result.parserDiagnostics.isEmpty());
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::FormattedStringExpression));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::FormattedStringText));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::FormattedStringInterpolation));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::FormattedStringFormat));
}

void TaifParserTest::recoveryPreservesLaterDeclarations() {
    const QString source = QStringLiteral(
        "اذا صح\n"
        "\tاطبع(1)\n"
        "دالة بعد_الخطأ():\n"
        "\tارجع 9\n");
    const ParseResult result = TaifParser().parse(source);

    QVERIFY(!result.parserDiagnostics.isEmpty());
    QVERIFY(hasDiagnostic(result, QStringLiteral("PAR001")));
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::FunctionDeclaration));
    QCOMPARE(result.syntaxTree->tokens().constLast().kind, TokenKind::EndOfFile);
}

void TaifParserTest::lexerDiagnosticsAreRetained() {
    const ParseResult result = TaifParser().parse(QStringLiteral("س = \"غير منته\n"));

    QVERIFY(!result.lexicalDiagnostics.isEmpty());
    QVERIFY(result.syntaxTree != nullptr);
    QVERIFY(result.ast != nullptr);
    QCOMPARE(result.syntaxTree->tokens().constLast().kind, TokenKind::EndOfFile);
}

void TaifParserTest::reparseFallsBackToAnEquivalentFreshSnapshot() {
    const TaifParser parser;
    const ParseResult before = parser.parse(QStringLiteral("س = 1\n"), 4);
    TextEdit edit;
    edit.replacedRange = {{4, 1, 5}, {5, 1, 6}};
    edit.insertedText = QStringLiteral("2");
    edit.baseRevision = 4;
    edit.resultRevision = 5;

    const IncrementalParseResult incremental = parser.reparse(
        before, QStringLiteral("س = 2\n"), edit);
    const ParseResult fresh = parser.parse(QStringLiteral("س = 2\n"), 5);

    QVERIFY(incremental.usedFullReparseFallback);
    QCOMPARE(incremental.reusedSyntaxNodeCount, qsizetype(0));
    QCOMPARE(incremental.result.documentRevision, fresh.documentRevision);
    QCOMPARE(incremental.result.ast->nodes().size(), fresh.ast->nodes().size());
    QCOMPARE(incremental.result.parserDiagnostics.size(), fresh.parserDiagnostics.size());
}

void TaifParserTest::statusCorpusProducesAFiniteTree() {
    QFile file(QFINDTESTDATA("../lexer/data/Status.alif"));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    const QString corpus = QString::fromUtf8(file.readAll());

    const ParseResult result = TaifParser().parse(corpus, 7);
    QVERIFY(result.syntaxTree != nullptr);
    QVERIFY(result.ast != nullptr);
    QCOMPARE(result.documentRevision, quint64(7));
    QCOMPARE(result.syntaxTree->tokens().constLast().kind, TokenKind::EndOfFile);
    QVERIFY(result.parserDiagnostics.size() < 256);
    QVERIFY(result.ast->nodes().size() > 100);
}

QTEST_GUILESS_MAIN(TaifParserTest)
#include "tst_TaifParser.moc"
