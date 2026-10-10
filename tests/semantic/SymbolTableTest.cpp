#include "AlifSymbolTable.h"

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
    void wildcardImportsDoNotDeclareAnAsteriskSymbol();
    void forTargetBindsAndResolvesInTheLoopBody();
    void tupleComprehensionVariablesResolveInTheirScope_data();
    void tupleComprehensionVariablesResolveInTheirScope();
    void comprehensionFiltersResolveBoundNames_data();
    void comprehensionFiltersResolveBoundNames();
    void comprehensionFiltersStillDiagnoseUnknownNames();
    void classesExposeMethodsAndFieldsThroughConstructorInstances();
    void memberReferencesRemainExternalUntilTypeAnalysis();
    void memberAssignmentsAreValidTargets_data();
    void memberAssignmentsAreValidTargets();
    void memberAssignmentPreservesKnownClassAttribute();
    void invalidAssignmentTargetsStillWarn();
    void parametersRemainVisibleAtIncompleteBodyEnd();
    void editorQueriesReturnScopedDefinitionsAndReferences();
    void malformedParserInputStillBuildsAFiniteSemanticModel();
    void enclosingSymbolPathTracksNestedClassAndFunctionScopes();
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

void SymbolTableTest::wildcardImportsDoNotDeclareAnAsteriskSymbol() {
    const SemanticFixture fixture = analyze(QStringLiteral(
        "من مكتبة استورد *\n"
        "س = 1\n"
        "اطبع(س)\n"));

    QVERIFY(fixture.parse.lexicalDiagnostics.isEmpty());
    QVERIFY(fixture.parse.parserDiagnostics.isEmpty());
    QVERIFY(fixture.model->diagnostics().isEmpty());
    QVERIFY(findSymbol(*fixture.model, QStringLiteral("*")) == nullptr);
    QVERIFY(findSymbol(*fixture.model, QStringLiteral("مكتبة")) == nullptr);
    const Symbol* local = findSymbol(*fixture.model, QStringLiteral("س"), SymbolKind::Local);
    QVERIFY(local != nullptr);
    QVERIFY(hasResolvedReference(*fixture.model, QStringLiteral("س"), local->id));
}

void SymbolTableTest::forTargetBindsAndResolvesInTheLoopBody() {
    const SemanticFixture fixture = analyze(QStringLiteral(
        "لكل ب في مدى(5):\n"
        "\tاطبع(ب)\n"));

    QVERIFY(fixture.parse.parserDiagnostics.isEmpty());
    const Symbol* loopVariable = findSymbol(*fixture.model, QStringLiteral("ب"),
                                            SymbolKind::LoopVariable);
    QVERIFY(loopVariable != nullptr);
    QVERIFY(hasResolvedReference(*fixture.model, QStringLiteral("ب"), loopVariable->id));
}

void SymbolTableTest::tupleComprehensionVariablesResolveInTheirScope_data() {
    QTest::addColumn<QString>("expression");
    QTest::newRow("bare") << QStringLiteral("متغير,متغير2 لكل متغير, متغير2 في تعبير");
    QTest::newRow("list") << QStringLiteral("[متغير,متغير2 لكل متغير, متغير2 في تعبير]");
    QTest::newRow("parenthesized") << QStringLiteral("(متغير,متغير2 لكل متغير, متغير2 في تعبير)");
    QTest::newRow("binary-elements") << QStringLiteral("متغير + 1,متغير2 * 2 لكل متغير, متغير2 في تعبير");
    QTest::newRow("unary-elements") << QStringLiteral("-متغير,+متغير2 لكل متغير, متغير2 في تعبير");
}

void SymbolTableTest::tupleComprehensionVariablesResolveInTheirScope() {
    QFETCH(QString, expression);
    const QString source = QStringLiteral("تعبير = [(1, 2)]\n") + expression + QChar(u'\n');
    const SemanticFixture fixture = analyze(source);
    QVERIFY(fixture.parse.parserDiagnostics.isEmpty());
    QVERIFY(fixture.model->diagnostics().isEmpty());
    const Symbol* first = findSymbol(*fixture.model, QStringLiteral("متغير"),
                                     SymbolKind::ComprehensionVariable);
    const Symbol* second = findSymbol(*fixture.model, QStringLiteral("متغير2"),
                                      SymbolKind::ComprehensionVariable);
    QVERIFY(first != nullptr);
    QVERIFY(second != nullptr);
    QCOMPARE(first->declaringScope, second->declaringScope);
    QCOMPARE(fixture.model->scopes().at(first->declaringScope).kind, ScopeKind::Comprehension);
    for (const Symbol* variable : {first, second}) {
        bool read = false;
        bool write = false;
        for (const NameReference& reference : fixture.model->references()) {
            if (reference.name != variable->name) {
                continue;
            }
            QCOMPARE(reference.state, ResolutionState::Resolved);
            QCOMPARE(reference.resolvedSymbol, variable->id);
            read |= reference.kind == ReferenceKind::Read;
            write |= reference.kind == ReferenceKind::Write;
        }
        QVERIFY(read);
        QVERIFY(write);
        const qsizetype elementOffset = source.indexOf(variable->name);
        QVERIFY(fixture.model->visibleSymbolsAt(elementOffset).contains(variable->id));
        QVERIFY(!fixture.model->visibleSymbolsAt(0).contains(variable->id));
    }
}

void SymbolTableTest::comprehensionFiltersResolveBoundNames_data() {
    QTest::addColumn<QString>("filter");
    QTest::newRow("reported-filter") << QStringLiteral("خلية == 0");
    QTest::newRow("multiple-filters") << QStringLiteral("خلية == 0 اذا س > 1");
    QTest::newRow("boolean-filter") << QStringLiteral("خلية == 0 و س > 1");
    QTest::newRow("nested-comprehension") << QStringLiteral("طول([ع لكل ع في [خلية]]) > 0");
}

void SymbolTableTest::comprehensionFiltersResolveBoundNames() {
    QFETCH(QString, filter);
    const QString source = QStringLiteral("[س لكل س, خلية في تعداد(هذا.اللوح) اذا ")
        + filter + QStringLiteral("]\n");
    const SemanticFixture fixture = analyze(source);
    QVERIFY(fixture.parse.lexicalDiagnostics.isEmpty());
    QVERIFY(fixture.parse.parserDiagnostics.isEmpty());
    QVERIFY(fixture.model->diagnostics().isEmpty());
    const Symbol* cell = findSymbol(*fixture.model, QStringLiteral("خلية"),
                                    SymbolKind::ComprehensionVariable);
    const Symbol* index = findSymbol(*fixture.model, QStringLiteral("س"),
                                     SymbolKind::ComprehensionVariable);
    QVERIFY(cell != nullptr);
    QVERIFY(index != nullptr);
    const NameReference* reference = fixture.model->referenceAt(source.lastIndexOf(QStringLiteral("خلية")));
    QVERIFY(reference != nullptr);
    QCOMPARE(reference->kind, ReferenceKind::Read);
    QCOMPARE(reference->state, ResolutionState::Resolved);
    QCOMPARE(reference->resolvedSymbol, cell->id);
    QVERIFY(fixture.model->visibleSymbolsAt(reference->range.begin.offset).contains(cell->id));
    QVERIFY(!fixture.model->visibleSymbolsAt(source.size()).contains(cell->id));
    QVERIFY(hasResolvedReference(*fixture.model, QStringLiteral("س"), index->id));
    if (filter.contains(QStringLiteral("لكل ع"))) {
        const Symbol* nested = findSymbol(*fixture.model, QStringLiteral("ع"),
                                          SymbolKind::ComprehensionVariable);
        QVERIFY(nested != nullptr);
        QVERIFY(hasResolvedReference(*fixture.model, QStringLiteral("ع"), nested->id));
    }
}

void SymbolTableTest::comprehensionFiltersStillDiagnoseUnknownNames() {
    const QString source = QStringLiteral(
        "[س لكل س, خلية في تعداد(هذا.اللوح) اذا مجهول == 0]\n");
    const SemanticFixture fixture = analyze(source);
    QVERIFY(fixture.parse.parserDiagnostics.isEmpty());
    bool foundUnknown = false;
    for (const SemanticDiagnostic& diagnostic : fixture.model->diagnostics()) {
        if (diagnostic.code == QStringLiteral("يدل001")
            && diagnostic.message.contains(QStringLiteral("مجهول"))) {
            QCOMPARE(diagnostic.range.begin.offset, source.indexOf(QStringLiteral("مجهول")));
            foundUnknown = true;
        }
    }
    QVERIFY(foundUnknown);
    QCOMPARE(fixture.model->diagnostics().size(), qsizetype(1));
}

void SymbolTableTest::classesExposeMethodsAndFieldsThroughConstructorInstances() {
    const SemanticFixture fixture = analyze(QStringLiteral(
        "صنف سيارة:\n"
        "\tلون = 0\n"
        "\tدالة تهيئة(هذا, لون):\n"
        "\t\tهذا.لون = لون\n"
        "\tدالة تغيير_لون_السيارة(هذا, لون_جديد):\n"
        "\t\tهذا.لون = لون_جديد\n"
        "تويوتا = سيارة()\n"
        "تويوتا.لون\n"));

    const Symbol* car = findSymbol(*fixture.model, QStringLiteral("سيارة"), SymbolKind::Class);
    const Symbol* toyota = findSymbol(*fixture.model, QStringLiteral("تويوتا"), SymbolKind::Local);
    QVERIFY(car != nullptr);
    QVERIFY(toyota != nullptr);
    QCOMPARE(toyota->instanceClass, car->id);

    const QVector<SymbolId> classMembers = fixture.model->membersOfClass(car->id);
    const QVector<SymbolId> instanceMembers = fixture.model->membersOfReceiver(toyota->id);
    QVERIFY(classMembers == instanceMembers);

    QStringList names;
    for (const SymbolId memberId : instanceMembers) {
        const Symbol* member = fixture.model->symbol(memberId);
        QVERIFY(member != nullptr);
        names.append(member->name);
    }
    QVERIFY(names.contains(QStringLiteral("لون")));
    QVERIFY(names.contains(QStringLiteral("تهيئة")));
    QVERIFY(names.contains(QStringLiteral("تغيير_لون_السيارة")));
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

void SymbolTableTest::memberAssignmentsAreValidTargets_data() {
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("unresolvedReceiver");
    QTest::newRow("exact-report") << QStringLiteral("شبكة_عصبية.معدل_التعلم = 0.005\n") << true;
    QTest::newRow("declared-receiver") << QStringLiteral("شبكة_عصبية = عدم\nشبكة_عصبية.معدل_التعلم = 0.005\n") << false;
    QTest::newRow("augmented-assignment") << QStringLiteral("شبكة_عصبية = عدم\nشبكة_عصبية.معدل_التعلم += 0.005\n") << false;
    QTest::newRow("nested-member") << QStringLiteral("شبكة_عصبية = عدم\nشبكة_عصبية.طبقة.معدل_التعلم = 0.005\n") << false;
    QTest::newRow("call-receiver") << QStringLiteral("دالة شبكة_عصبية():\n\tارجع عدم\nشبكة_عصبية().معدل_التعلم = 0.005\n") << false;
    QTest::newRow("indexed-receiver") << QStringLiteral("شبكة_عصبية = []\nشبكة_عصبية[0].معدل_التعلم = 0.005\n") << false;
}

void SymbolTableTest::memberAssignmentsAreValidTargets() {
    QFETCH(QString, source);
    QFETCH(bool, unresolvedReceiver);
    const SemanticFixture fixture = analyze(source);
    QVERIFY(fixture.parse.lexicalDiagnostics.isEmpty());
    QVERIFY(fixture.parse.parserDiagnostics.isEmpty());
    QVERIFY(!hasDiagnostic(*fixture.model, QStringLiteral("يدل003")));
    QCOMPARE(hasDiagnostic(*fixture.model, QStringLiteral("يدل001")), unresolvedReceiver);
    if (!unresolvedReceiver) {
        QVERIFY(fixture.model->diagnostics().isEmpty());
    }
    QVERIFY(findSymbol(*fixture.model, QStringLiteral("معدل_التعلم")) == nullptr);
    const NameReference* member = fixture.model->referenceAt(source.lastIndexOf(QStringLiteral("معدل_التعلم")));
    QVERIFY(member != nullptr);
    QCOMPARE(member->kind, ReferenceKind::Member);
    QCOMPARE(member->state, ResolutionState::External);
    if (!unresolvedReceiver) {
        const Symbol* receiver = findSymbol(*fixture.model, QStringLiteral("شبكة_عصبية"));
        QVERIFY(receiver != nullptr);
        QVERIFY(hasResolvedReference(*fixture.model, receiver->name, receiver->id));
    }
}

void SymbolTableTest::memberAssignmentPreservesKnownClassAttribute() {
    const QString source = QStringLiteral(
        "صنف شبكة:\n"
        "\tدالة تهيئة(هذا):\n"
        "\t\tهذا.معدل_التعلم = 0.1\n"
        "شبكة_عصبية = شبكة()\n"
        "شبكة_عصبية.معدل_التعلم = 0.005\n");
    const SemanticFixture fixture = analyze(source);
    QVERIFY(fixture.parse.parserDiagnostics.isEmpty());
    QVERIFY(fixture.model->diagnostics().isEmpty());
    const Symbol* attribute = findSymbol(*fixture.model, QStringLiteral("معدل_التعلم"), SymbolKind::Attribute);
    QVERIFY(attribute != nullptr);
    const NameReference* member = fixture.model->referenceAt(source.lastIndexOf(attribute->name));
    QVERIFY(member != nullptr);
    QCOMPARE(member->state, ResolutionState::Resolved);
    QCOMPARE(member->resolvedSymbol, attribute->id);
    QCOMPARE(fixture.model->scopes().at(attribute->declaringScope).kind, ScopeKind::Class);
    qsizetype attributeCount = 0;
    for (const Symbol& symbol : fixture.model->symbols()) {
        if (symbol.name == attribute->name) {
            ++attributeCount;
        }
    }
    QCOMPARE(attributeCount, qsizetype(1));
}

void SymbolTableTest::invalidAssignmentTargetsStillWarn() {
    for (const QString& source : {QStringLiteral("1 = 0.005\n"),
                                  QStringLiteral("اطبع() = 0.005\n"),
                                  QStringLiteral("1 + 2 = 0.005\n")}) {
        const SemanticFixture fixture = analyze(source);
        QVERIFY(fixture.parse.parserDiagnostics.isEmpty());
        QVERIFY(hasDiagnostic(*fixture.model, QStringLiteral("يدل003")));
    }
}

void SymbolTableTest::parametersRemainVisibleAtIncompleteBodyEnd() {
    const QString source = QStringLiteral("دالة تعبير(معاملات):\n\tمعا");
    const SemanticFixture fixture = analyze(source);

    const Symbol* parameter = findSymbol(*fixture.model, QStringLiteral("معاملات"),
                                          SymbolKind::Parameter);
    QVERIFY(parameter != nullptr);

    const QVector<SymbolId> visible = fixture.model->visibleSymbolsAt(source.size());
    QVERIFY(visible.contains(parameter->id));
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

void SymbolTableTest::enclosingSymbolPathTracksNestedClassAndFunctionScopes() {
    const QString source = QStringLiteral(
        "صنف سيارة:\n"
        "\tدالة تغيير_اللون(هذا):\n"
        "\t\tاطبع(هذا)\n"
        "اطبع(\"خارج\")\n");
    const SemanticFixture fixture = analyze(source);
    QVERIFY(fixture.model != nullptr);

    const qsizetype insideMethod = source.indexOf(QStringLiteral("اطبع(هذا)"));
    const QVector<SemanticBreadcrumb> path = fixture.model->enclosingSymbolPathAt(insideMethod);
    QCOMPARE(path.size(), 2);
    QCOMPARE(path.at(0).kind, SymbolKind::Class);
    QCOMPARE(path.at(0).name, QStringLiteral("سيارة"));
    QCOMPARE(path.at(1).kind, SymbolKind::Function);
    QCOMPARE(path.at(1).name, QStringLiteral("تغيير_اللون"));

    const qsizetype outsideDeclarations = source.lastIndexOf(QStringLiteral("اطبع(\"خارج\")"));
    QVERIFY(fixture.model->enclosingSymbolPathAt(outsideDeclarations).isEmpty());
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
    QVERIFY(fixture.model->diagnostics().isEmpty());
}

QTEST_GUILESS_MAIN(SymbolTableTest)
#include "SymbolTableTest.moc"
