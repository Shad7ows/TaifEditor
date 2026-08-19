#include "SemanticCompletionProvider.h"

namespace {

QString descriptionForSymbol(const SymbolKind kind) {
    switch (kind) {
    case SymbolKind::Function: return QStringLiteral("دالة");
    case SymbolKind::Class: return QStringLiteral("صنف");
    case SymbolKind::Field: return QStringLiteral("خاصية");
    case SymbolKind::Parameter: return QStringLiteral("معامل");
    case SymbolKind::ImportModule: return QStringLiteral("وحدة مستوردة");
    case SymbolKind::ImportMember: return QStringLiteral("اسم مستورد");
    case SymbolKind::Builtin: return QStringLiteral("مدمج");
    case SymbolKind::LoopVariable: return QStringLiteral("متغير حلقة");
    case SymbolKind::ComprehensionVariable: return QStringLiteral("متغير استيعاب");
    default: return QStringLiteral("متغير محلي");
    }
}

} // namespace

QVector<CompletionItem> SemanticCompletionProvider::suggestions(
    const QString& prefix,
    const qsizetype cursorOffset,
    const std::shared_ptr<const SemanticModel>& semantic,
    const bool moduleAndPreludeOnly) const {
    QVector<CompletionItem> items;
    if (!semantic) {
        return items;
    }

    const QVector<SymbolId> visible = semantic->visibleSymbolsAt(cursorOffset);
    items.reserve(visible.size());
    for (const SymbolId id : visible) {
        const Symbol* symbol = semantic->symbol(id);
        if (symbol == nullptr || !symbol->name.startsWith(prefix, Qt::CaseInsensitive)) {
            continue;
        }
        if (moduleAndPreludeOnly && symbol->declaringScope != semantic->moduleScope()
            && symbol->declaringScope != semantic->preludeScope()) {
            continue;
        }
        CompletionItem item;
        item.label = symbol->name;
        item.completion = symbol->name;
        item.description = descriptionForSymbol(symbol->kind);
        item.type = CompletionType::SemanticSymbol;
        items.append(std::move(item));
    }
    return items;
}

QVector<CompletionItem> SemanticCompletionProvider::memberSuggestions(
    const QString& receiverName,
    const QString& prefix,
    const qsizetype cursorOffset,
    const std::shared_ptr<const SemanticModel>& semantic,
    const bool moduleAndPreludeOnly) const {
    QVector<CompletionItem> items;
    if (!semantic || receiverName.isEmpty()) {
        return items;
    }

    SymbolId receiver = InvalidSymbolId;
    const QVector<SymbolId> visible = semantic->visibleSymbolsAt(cursorOffset);
    for (const SymbolId candidateId : visible) {
        const Symbol* candidate = semantic->symbol(candidateId);
        if (candidate == nullptr || candidate->name != receiverName) {
            continue;
        }
        if (moduleAndPreludeOnly && candidate->declaringScope != semantic->moduleScope()
            && candidate->declaringScope != semantic->preludeScope()) {
            continue;
        }
        receiver = candidateId;
        break;
    }
    if (receiver == InvalidSymbolId) {
        return items;
    }

    const QVector<SymbolId> members = semantic->membersOfReceiver(receiver);
    items.reserve(members.size());
    for (const SymbolId memberId : members) {
        const Symbol* member = semantic->symbol(memberId);
        if (member == nullptr || !member->name.startsWith(prefix, Qt::CaseInsensitive)) {
            continue;
        }
        CompletionItem item;
        item.label = member->name;
        item.completion = member->name;
        item.description = descriptionForSymbol(member->kind);
        item.type = CompletionType::SemanticSymbol;
        items.append(std::move(item));
    }
    return items;
}
