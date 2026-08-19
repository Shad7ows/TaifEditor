#include "EditorAnalysisController.h"
#include "SemanticPresentationAdapter.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

class EditorAnalysisControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void tierZeroImmediatelyUpdatesRevisionAndDefersWork();
    void rapidEditsCoalesceBothDeferredTiers();
    void latestWorkerSnapshotWinsAfterRevisionChange();
    void semanticPresentationClassifiesDeclarationsAndDiagnostics();
};

void EditorAnalysisControllerTest::tierZeroImmediatelyUpdatesRevisionAndDefersWork() {
    EditorAnalysisController controller;
    QSignalSpy fastSpy(&controller, &EditorAnalysisController::fastPassRequested);
    QSignalSpy semanticSpy(&controller, &EditorAnalysisController::semanticSnapshotRequested);

    controller.documentChanged(3, 0, 1);
    QCOMPARE(controller.currentRevision(), quint64(1));
    QCOMPARE(fastSpy.count(), 0);
    QCOMPARE(semanticSpy.count(), 0);

    QTRY_COMPARE_WITH_TIMEOUT(fastSpy.count(), 1, 250);
    QCOMPARE(semanticSpy.count(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(semanticSpy.count(), 1, 250);
}

void EditorAnalysisControllerTest::rapidEditsCoalesceBothDeferredTiers() {
    EditorAnalysisController controller;
    QSignalSpy fastSpy(&controller, &EditorAnalysisController::fastPassRequested);
    QSignalSpy semanticSpy(&controller, &EditorAnalysisController::semanticSnapshotRequested);

    controller.documentChanged(0, 0, 1);
    controller.documentChanged(1, 0, 1);
    controller.documentChanged(2, 0, 1);
    QCOMPARE(controller.currentRevision(), quint64(3));

    QTRY_COMPARE_WITH_TIMEOUT(fastSpy.count(), 1, 275);
    QTRY_COMPARE_WITH_TIMEOUT(semanticSpy.count(), 1, 275);
    QCOMPARE(fastSpy.at(0).at(0).toULongLong(), quint64(3));
    QCOMPARE(semanticSpy.at(0).at(0).toULongLong(), quint64(3));
}

void EditorAnalysisControllerTest::latestWorkerSnapshotWinsAfterRevisionChange() {
    EditorAnalysisController controller;
    QSignalSpy appliedSpy(&controller, &EditorAnalysisController::analysisApplied);

    controller.documentChanged(0, 0, 1);
    controller.submitSourceSnapshot(1, QStringLiteral("س = 1\n"));
    controller.documentChanged(0, 1, 1);
    controller.submitSourceSnapshot(2, QStringLiteral("س = 2\n"));

    QTRY_VERIFY_WITH_TIMEOUT(appliedSpy.count() >= 1, 1500);
    const LanguageAnalysisSnapshotPtr snapshot = controller.currentSnapshot();
    QVERIFY(snapshot != nullptr);
    QCOMPARE(snapshot->revision, quint64(2));
}

void EditorAnalysisControllerTest::semanticPresentationClassifiesDeclarationsAndDiagnostics() {
    const QString source = QStringLiteral(
        "دالة جمع(س):\n"
        "\tارجع مجهول + س\n");
    const LexResult lexical = TaifLexer().lex(source);
    const ParseResult parse = TaifParser().parse(source, lexical, 1);
    const SymbolTableInput input {*parse.ast, parse.parserDiagnostics, 1};
    const std::shared_ptr<const SemanticModel> semantic = SymbolTableBuilder().build(input);
    const QVector<PresentationSpan> spans = SemanticPresentationAdapter().classify(
        lexical, parse, semantic);

    bool foundFunction = false;
    bool foundParameter = false;
    bool foundUnresolved = false;
    for (const PresentationSpan& span : spans) {
        foundFunction = foundFunction || span.classification == PresentationClass::FunctionDeclaration;
        foundParameter = foundParameter || span.classification == PresentationClass::Parameter;
        foundUnresolved = foundUnresolved || span.classification == PresentationClass::UnresolvedName;
    }
    QVERIFY(foundFunction);
    QVERIFY(foundParameter);
    QVERIFY(foundUnresolved);
}

QTEST_GUILESS_MAIN(EditorAnalysisControllerTest)
#include "tst_EditorAnalysisController.moc"
