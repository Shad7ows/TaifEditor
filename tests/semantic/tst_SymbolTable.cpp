#include "SymbolTable.h"

#include <QtCore/QFile>
#include <QtTest/QTest>

namespace {

struct SemanticFixture final {
    ParseResult parse;
    std::shared_ptr<const SemanticModel> model;
};

SemanticFixture analyze(const QString& source, const quint64 revision = 1) {
    const ParseResult parse = TaifParser().parse(source, revision);
    const SymbolTableInput input {*parse.ast, parse.parserDiagnostics, revision};
    return {parse, SymbolTableBuilder().build(input)};
}

const Symbol* findSymbol(const SemanticModel& model, const QString& name,
                         const SymbolKind kind = SymbolKind::Error) {
    for (const Symbol& symbol : model.symbols()) {
        if (symbol.name == name && (kind == SymbolKind::Error || symbol.kind == kind)) {
            return &symbol;
        }
    }
    return nullptr;
}

bool hasDiagnostic(const SemanticModel& model, const QString& code) {
    for (const SemanticDiagnostic& diagnostic : model.diagnostics()) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

bool hasResolvedReference(const SemanticModel& model, const QString& name,
                          const SymbolId expectedSymbol) {
    for (const NameReference& reference : model.references()) {
        if (reference.name == name && reference.state == ResolutionState::Resolved
            && reference.resolvedSymbol == expectedSymbol) {
            return true;
        }
    }
    return false;
}

} // namespace

class SymbolTableTest final : public QObject {
    Q_OBJECT

private slots:
    void emptyModuleCreatesPreludeAndModuleScopes();
    void nestedScopesResolveShadowingClosuresAndRecursion();
    void assignmentsImportsAndBuiltinsBecomeVisibleSymbols();
    void duplicateAndUnresolvedNamesProduceRecoverableDiagnostics();
    void memberReferencesRemainExternalUntilTypeAnalysis();
    void editorQueriesReturnScopedDefinitionsAndReferences();
    void malformedParserInputStillBuildsAFiniteSemanticModel();
    void statusCorpusBuildsAFiniteRevisionedModel();
};

void SymbolTableTest::emptyModuleCreatesPreludeAndModuleScopes() {
    const SemanticFixture fixture = analyze(QString(), 9);

    QVERIFY(fixture.model != nullptr);
    QCOMPARE(fixture.model->documentRevision(), quint64(9));
    QVERIFY(fixture.model->preludeScope() != InvalidScopeId);
    QVERIFY(fixture.model->moduleScope() != InvalidScopeId);
    QCOMPARE(fixture.model->scopes().size(), qsizetype(2));
    QVERIFY(findSymbol(*fixture.model, QStringLiteral("اطبع"), SymbolKind::Builtin) != nullptr);
}

void SymbolTableTest::nestedScopesResolveShadowingClosuresAndRecursion() {
    const QString source = QStringLiteral(
        "دالة خارجي(س):\n"
        "\tص = س\n"
        "\tدالة داخلي(س):\n"
        "\t\tارجع داخلي(س) + ص\n"
        "\tارجع داخلي(ص)\n");
    const SemanticFixture fixture = analyze(source);

    const Symbol* outer = findSymbol(*fixture.model, QStringLiteral("خارجي"), SymbolKind::Function);
    const Symbol* inner = findSymbol(*fixture.model, QStringLiteral("داخلي"), SymbolKind::Function);
    const Symbol* outerLocal = findSymbol(*fixture.model, QStringLiteral("ص"), SymbolKind::Local);
    QVERIFY(outer != nullptr);
    QVERIFY(inner != nullptr);
    QVERIFY(outerLocal != nullptr);
    QVERIFY(hasResolvedReference(*fixture.model, QStringLiteral("داخلي"), inner->id));
    QVERIFY(hasResolvedReference(*fixture.model, QStringLiteral("ص"), outerLocal->id));

    qsizetype functionScopes = 0;
    for (const Scope& scope : fixture.model->scopes()) {
        if (scope.kind == ScopeKind::Function) {
            ++functionScopes;
        }
    }
    QCOMPARE(functionScopes, qsizetype(2));
}

void SymbolTableTest::assignmentsImportsAndBuiltinsBecomeVisibleSymbols() {
    const QString source = QStringLiteral(
        "استورد مكتبة.فرع\n"
        "من مكتبة استورد شيء\n"
        "س = شيء\n"
        "اطبع(س)\n");
    const SemanticFixture fixture = analyze(source);

    const Symbol* imported = findSymbol(*fixture.model, QStringLiteral("شيء"), SymbolKind::ImportMember);
    const Symbol* local = findSymbol(*fixture.model, QStringLiteral("س"), SymbolKind::Local);
    const Symbol* builtin = findSymbol(*fixture.model, QStringLiteral("اطبع"), SymbolKind::Builtin);
    QVERIFY(imported != nullptr);
    QVERIFY(local != nullptr);
    QVERIFY(builtin != nullptr);
    QVERIFY(hasResolvedReference(*fixture.model, QStringLiteral("شيء"), imported->id));
    QVERIFY(hasResolvedReference(*fixture.model, QStringLiteral("اطبع"), builtin->id));
}

void SymbolTableTest::duplicateAndUnresolvedNamesProduceRecoverableDiagnostics() {
    const SemanticFixture fixture = analyze(QStringLiteral(
        "س = 1\n"
        "س = 2\n"
        "اطبع(مجهول)\n"));

    QVERIFY(hasDiagnostic(*fixture.model, QStringLiteral("SEM002")));
    QVERIFY(hasDiagnostic(*fixture.model, QStringLiteral("SEM001")));
    QVERIFY(fixture.model->references().size() >= 3);
}

void SymbolTableTest::memberReferencesRemainExternalUntilTypeAnalysis() {
    const SemanticFixture fixture = analyze(QStringLiteral(
        "س = 1\n"
        "س.خاصية\n"));

    bool foundMember = false;
    for (const NameReference& reference : fixture.model->references()) {
        if (reference.name == QStringLiteral("خاصية")) {
            QCOMPARE(reference.kind, ReferenceKind::Member);
            QCOMPARE(reference.state, ResolutionState::External);
            foundMember = true;
        }
    }
    QVERIFY(foundMember);
}

void SymbolTableTest::editorQueriesReturnScopedDefinitionsAndReferences() {
    const QString source = QStringLiteral(
        "دالة جمع(س):\n"
        "\tص = س\n"
        "\tارجع ص\n");
    const SemanticFixture fixture = analyze(source, 4);
    const Symbol* local = findSymbol(*fixture.model, QStringLiteral("ص"), SymbolKind::Local);
    QVERIFY(local != nullptr);

    const NameReference* reference = fixture.model->referenceAt(source.lastIndexOf(QStringLiteral("ص")));
    QVERIFY(reference != nullptr);
    QCOMPARE(reference->resolvedSymbol, local->id);
    QVERIFY(!fixture.model->referencesOf(local->id).isEmpty());

    const QVector<SymbolId> visible = fixture.model->visibleSymbolsAt(
        source.lastIndexOf(QStringLiteral("ص")));
    QVERIFY(visible.contains(local->id));

    const QVector<SymbolId> outline = fixture.model->documentSymbols();
    QVERIFY(!outline.isEmpty());
    const Symbol* declaration = fixture.model->symbol(outline.constFirst());
    QVERIFY(declaration != nullptr);
    QCOMPARE(declaration->name, QStringLiteral("جمع"));
}

void SymbolTableTest::malformedParserInputStillBuildsAFiniteSemanticModel() {
    const SemanticFixture fixture = analyze(QStringLiteral(
        "اذا صح\n"
        "\tس = 1\n"
        "دالة بعد_خطأ():\n"
        "\tارجع س\n"));

    QVERIFY(fixture.model != nullptr);
    QVERIFY(fixture.model->scopes().size() >= 2);
    QVERIFY(findSymbol(*fixture.model, QStringLiteral("بعد_خطأ"), SymbolKind::Function) != nullptr);
    QVERIFY(fixture.model->diagnostics().size() < 256);
}

void SymbolTableTest::statusCorpusBuildsAFiniteRevisionedModel() {
    QFile file(QFINDTESTDATA("../lexer/data/Status.alif"));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    const QString corpus = QString::fromUtf8(file.readAll());

    const SemanticFixture fixture = analyze(corpus, 17);
    QVERIFY(fixture.model != nullptr);
    QCOMPARE(fixture.model->documentRevision(), quint64(17));
    QVERIFY(fixture.model->scopes().size() >= 2);
    QVERIFY(fixture.model->symbols().size() > 20);
    QVERIFY(fixture.model->diagnostics().size() <= 97);
    QVERIFY(hasDiagnostic(*fixture.model, QStringLiteral("SEM999")));
}

QTEST_GUILESS_MAIN(SymbolTableTest)
#include "tst_SymbolTable.moc"
