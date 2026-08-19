#include "EditorAnalysisController.h"
#include "SemanticPresentationAdapter.h"
#include "SemanticCompletionProvider.h"
#include "AutoCompleteUI.h"
#include "CompletionContext.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <algorithm>

class EditorAnalysisControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void tierZeroImmediatelyUpdatesRevisionAndDefersWork();
    void rapidEditsCoalesceBothDeferredTiers();
    void latestWorkerSnapshotWinsAfterRevisionChange();
    void completionAnalysisRunsBeforeTheTierTwoTimer();
    void memberCompletionUsesClassesAndConstructorInstances();
    void completionContextNeverSelectsReceiverDot();
    void selfReceiverCompletionAndPresentation();
    void completionVisualsCoverLegacySemanticAndUnknownTypes();
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

void EditorAnalysisControllerTest::completionAnalysisRunsBeforeTheTierTwoTimer() {
    EditorAnalysisController controller;
    QSignalSpy appliedSpy(&controller, &EditorAnalysisController::analysisApplied);
    QSignalSpy tierTwoSpy(&controller, &EditorAnalysisController::semanticSnapshotRequested);

    controller.documentChanged(0, 0, 1);
    controller.requestCompletionAnalysis(1, QStringLiteral("س = 1\n"));

    QTRY_VERIFY_WITH_TIMEOUT(appliedSpy.count() >= 1, 250);
    QCOMPARE(tierTwoSpy.count(), 0);
    const LanguageAnalysisSnapshotPtr snapshot = controller.currentSnapshot();
    QVERIFY(snapshot != nullptr);
    QCOMPARE(snapshot->revision, quint64(1));
}

void EditorAnalysisControllerTest::memberCompletionUsesClassesAndConstructorInstances() {
    const QString source = QStringLiteral(
        "صنف سيارة:\n"
        "\tلون = 0\n"
        "\tدالة تهيئة(هذا, لون):\n"
        "\t\tهذا.لون = لون\n"
        "\tدالة تغيير_لون_السيارة(هذا, لون_جديد):\n"
        "\t\tهذا.لون = لون_جديد\n"
        "تويوتا = سيارة()\n");
    const LexResult lexical = TaifLexer().lex(source);
    const ParseResult parse = TaifParser().parse(source, lexical, 1);
    const SymbolTableInput input {*parse.ast, parse.parserDiagnostics, 1};
    const std::shared_ptr<const SemanticModel> semantic = SymbolTableBuilder().build(input);
    const qsizetype offset = source.size();
    const qsizetype classReceiverOffset = source.lastIndexOf(QStringLiteral("سيارة"));
    const qsizetype instanceReceiverOffset = source.indexOf(QStringLiteral("تويوتا"));

    const SemanticCompletionProvider provider;
    const QVector<CompletionItem> classItems = provider.memberSuggestions(
        QStringLiteral("سيارة"), QString(), classReceiverOffset, offset, semantic);
    const QVector<CompletionItem> instanceItems = provider.memberSuggestions(
        QStringLiteral("تويوتا"), QString(), instanceReceiverOffset, offset, semantic);

    QStringList classNames;
    for (const CompletionItem& item : classItems) {
        classNames.append(item.completion);
    }
    QStringList instanceNames;
    for (const CompletionItem& item : instanceItems) {
        instanceNames.append(item.completion);
    }
    QVERIFY(classNames.contains(QStringLiteral("لون")));
    QVERIFY(classNames.contains(QStringLiteral("تهيئة")));
    QVERIFY(classNames.contains(QStringLiteral("تغيير_لون_السيارة")));
    QCOMPARE(instanceNames, classNames);
    const auto field = std::find_if(instanceItems.cbegin(), instanceItems.cend(),
                                    [](const CompletionItem& item) {
                                        return item.completion == QStringLiteral("لون");
                                    });
    QVERIFY(field != instanceItems.cend());
    QCOMPARE(field->semanticKind, CompletionSemanticKind::Field);
}

void EditorAnalysisControllerTest::completionContextNeverSelectsReceiverDot() {
    const QString emptyMember = QStringLiteral("تويوتا.");
    const CompletionContext empty = completionContextAt(emptyMember, emptyMember.size(), 100);
    QVERIFY(empty.isMemberAccess);
    QCOMPARE(empty.receiver, QStringLiteral("تويوتا"));
    QCOMPARE(empty.replacementBegin, qsizetype(100 + emptyMember.size()));
    QCOMPARE(empty.replacementEnd, empty.replacementBegin);

    const QString partialMember = QStringLiteral("تويوتا.تغ");
    const CompletionContext partial = completionContextAt(partialMember, partialMember.size(), 200);
    QVERIFY(partial.isMemberAccess);
    QCOMPARE(partial.prefix, QStringLiteral("تغ"));
    QCOMPARE(partialMember.mid(partial.replacementBegin - 200,
                               partial.replacementEnd - partial.replacementBegin),
             QStringLiteral("تغ"));
    QVERIFY(partial.replacementBegin > 200 + partial.receiver.size());
}

void EditorAnalysisControllerTest::selfReceiverCompletionAndPresentation() {
    const QString source = QStringLiteral(
        "صنف سيارة:\n"
        "\tلون = 0\n"
        "\tدالة تهيئة(هذا, لون):\n"
        "\t\tهذا.لون = لون\n"
        "\tدالة تغيير_لون_السيارة(هذا, لون_جديد):\n"
        "\t\tهذا.لون = لون_جديد\n");
    const LexResult lexical = TaifLexer().lex(source);
    const ParseResult parse = TaifParser().parse(source, lexical, 1);
    const SymbolTableInput input {*parse.ast, parse.parserDiagnostics, 1};
    const std::shared_ptr<const SemanticModel> semantic = SymbolTableBuilder().build(input);
    const qsizetype receiverOffset = source.indexOf(QStringLiteral("هذا.لون"));
    QVERIFY(receiverOffset >= 0);

    const SemanticCompletionProvider provider;
    const QVector<CompletionItem> selfItems = provider.memberSuggestions(
        QStringLiteral("هذا"), QString(), receiverOffset, receiverOffset + 3, semantic);
    QStringList names;
    for (const CompletionItem& item : selfItems) {
        names.append(item.completion);
    }
    QVERIFY(names.contains(QStringLiteral("لون")));
    QVERIFY(names.contains(QStringLiteral("تهيئة")));
    QVERIFY(names.contains(QStringLiteral("تغيير_لون_السيارة")));

    const QVector<PresentationSpan> spans = SemanticPresentationAdapter().classify(
        lexical, parse, semantic);
    bool hasSelfReceiver = false;
    for (const PresentationSpan& span : spans) {
        if (span.classification == PresentationClass::SelfReceiver) {
            hasSelfReceiver = true;
            break;
        }
    }
    QVERIFY(hasSelfReceiver);
}

void EditorAnalysisControllerTest::completionVisualsCoverLegacySemanticAndUnknownTypes() {
    const CompletionVisual keyword = completionVisual(CompletionType::Keyword);
    const CompletionVisual semanticMethod = completionVisual(
        CompletionType::SemanticSymbol, CompletionSemanticKind::Function);
    const CompletionVisual semanticField = completionVisual(
        CompletionType::SemanticSymbol, CompletionSemanticKind::Field);
    const CompletionVisual fallback = completionVisual(
        static_cast<CompletionType>(999), CompletionSemanticKind::Unknown);

    QVERIFY(!keyword.icon.isEmpty());
    QCOMPARE(keyword.category, QStringLiteral("محجوزة"));
    QCOMPARE(semanticMethod.category, QStringLiteral("دالة"));
    QCOMPARE(semanticField.category, QStringLiteral("خاصية"));
    QVERIFY(semanticMethod.color.isValid());
    QVERIFY(semanticField.color.isValid());
    QCOMPARE(fallback.icon, QStringLiteral("?"));
    QCOMPARE(fallback.category, QStringLiteral("اقتراح"));

    CompletionModel model;
    model.updateData({{QStringLiteral("لون"), QStringLiteral("لون"),
                       QStringLiteral("خاصية"), CompletionType::SemanticSymbol,
                       CompletionSemanticKind::Field}});
    const QModelIndex index = model.index(0, 0);
    QCOMPARE(index.data(Qt::UserRole + 3).toInt(),
             static_cast<int>(CompletionSemanticKind::Field));
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
