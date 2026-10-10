#include "AlifParser.h"

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
    void decoratorsPrefixDeclarationsWithoutDiagnostics();
    void forHeaderTreatsInAsAStructuralDelimiter();
    void fromImportAcceptsWildcard_data();
    void fromImportAcceptsWildcard();
    void fromImportPreservesNamedImports();
    void expressionsUsePrattPostfixAndPrecedenceParsing();
    void formattedStringCreatesStructuredAst();
    void recoveryPreservesLaterDeclarations();
    void lexerDiagnosticsAreRetained();
    void reparseFallsBackToAnEquivalentFreshSnapshot();
    void passStatementIsParsedWithoutError();
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

void TaifParserTest::decoratorsPrefixDeclarationsWithoutDiagnostics() {
    const ParseResult result = TaifParser().parse(
        QStringLiteral("@مميز\nدالة جمع(س):\n\tارجع س\n"));

    QVERIFY(result.syntaxTree != nullptr);
    QVERIFY(result.ast != nullptr);
    QVERIFY(result.parserDiagnostics.isEmpty());
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::FunctionDeclaration));
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

void TaifParserTest::fromImportAcceptsWildcard_data() {
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("path");
    QTest::newRow("exact-report") << QStringLiteral("من مكتبة استورد *")
                                  << QStringLiteral("مكتبة");
    QTest::newRow("newline") << QStringLiteral("من مكتبة استورد *\n")
                             << QStringLiteral("مكتبة");
    QTest::newRow("dotted-path") << QStringLiteral("من مكتبة.فرع استورد *\n")
                                 << QStringLiteral("مكتبة.فرع");
    QTest::newRow("relative-path") << QStringLiteral("من .مكتبة استورد *\n")
                                   << QStringLiteral(".مكتبة");
    QTest::newRow("following-statement") << QStringLiteral("من مكتبة استورد *\nس = 1\n")
                                         << QStringLiteral("مكتبة");
}

void TaifParserTest::fromImportAcceptsWildcard() {
    QFETCH(QString, source);
    QFETCH(QString, path);
    const ParseResult result = TaifParser().parse(source);

    QVERIFY(result.lexicalDiagnostics.isEmpty());
    QVERIFY(result.parserDiagnostics.isEmpty());
    QVERIFY(!hasAstKind(*result.ast, AstNodeKind::ErrorExpression));
    const AstNode& statement = result.ast->node(result.ast->root().children.constFirst());
    QCOMPARE(statement.kind, AstNodeKind::FromImportStatement);
    QCOMPARE(statement.children.size(), qsizetype(2));
    QCOMPARE(statement.childRoles.size(), statement.children.size());
    QCOMPARE(statement.childRoles.at(0), AstChildRole::ImportPath);
    QCOMPARE(statement.childRoles.at(1), AstChildRole::ImportName);
    QCOMPARE(result.ast->node(statement.children.at(0)).text, path);
    const AstNode& wildcard = result.ast->node(statement.children.at(1));
    QCOMPARE(wildcard.kind, AstNodeKind::ImportWildcard);
    QCOMPARE(wildcard.text, QStringLiteral("*"));
    QCOMPARE(wildcard.range.begin.offset, source.indexOf(QChar(u'*')));
    QCOMPARE(wildcard.range.end.offset, wildcard.range.begin.offset + 1);
    const SyntaxNode& syntax = result.syntaxTree->nodes().at(wildcard.syntaxNode);
    QCOMPARE(syntax.kind, SyntaxKind::ImportWildcard);
    QCOMPARE(result.syntaxTree->tokens().at(syntax.firstToken).kind, TokenKind::Star);
    QCOMPARE(syntax.endToken, syntax.firstToken + 1);
    if (source.contains(QStringLiteral("س = 1"))) {
        QCOMPARE(result.ast->root().children.size(), qsizetype(2));
        QVERIFY(hasAstKind(*result.ast, AstNodeKind::AssignmentStatement));
    }
}

void TaifParserTest::fromImportPreservesNamedImports() {
    const ParseResult result = TaifParser().parse(QStringLiteral("من مكتبة استورد س, ص\n"));

    QVERIFY(result.parserDiagnostics.isEmpty());
    const AstNode& statement = result.ast->node(result.ast->root().children.constFirst());
    QCOMPARE(statement.kind, AstNodeKind::FromImportStatement);
    QCOMPARE(statement.children.size(), qsizetype(3));
    QCOMPARE(statement.childRoles.size(), statement.children.size());
    QCOMPARE(statement.childRoles.at(0), AstChildRole::ImportPath);
    for (qsizetype index = 1; index < statement.children.size(); ++index) {
        QCOMPARE(statement.childRoles.at(index), AstChildRole::ImportName);
        QCOMPARE(result.ast->node(statement.children.at(index)).kind, AstNodeKind::NameExpression);
    }
    QCOMPARE(result.ast->node(statement.children.at(1)).text, QStringLiteral("س"));
    QCOMPARE(result.ast->node(statement.children.at(2)).text, QStringLiteral("ص"));
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
    QVERIFY(hasDiagnostic(result, QStringLiteral("عقد001")));
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

void TaifParserTest::passStatementIsParsedWithoutError() {
    const QString source = QStringLiteral(
        "دالة اختبار():\n"
        "\tاذا صح:\n"
        "\t\tمرر\n"
        "\tوالا:\n"
        "\t\tاطبع(1)\n");
    const ParseResult result = TaifParser().parse(source);

    QVERIFY(result.parserDiagnostics.isEmpty());
    QVERIFY(hasAstKind(*result.ast, AstNodeKind::PassStatement));
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
    QVERIFY(result.parserDiagnostics.isEmpty());
    QVERIFY(result.ast->nodes().size() > 100);
}

QTEST_GUILESS_MAIN(TaifParserTest)
#include "AlifParserTest.moc"
