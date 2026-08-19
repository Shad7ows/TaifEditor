#include "SemanticCompletionProvider.h"

namespace {

QString descriptionForSymbol(const SymbolKind kind) {
    switch (kind) {
    case SymbolKind::Function: return QStringLiteral("دالة");
    case SymbolKind::Class: return QStringLiteral("صنف");
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
    const std::shared_ptr<const SemanticModel>& semantic) const {
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
        CompletionItem item;
        item.label = symbol->name;
        item.completion = symbol->name;
        item.description = descriptionForSymbol(symbol->kind);
        item.type = CompletionType::SemanticSymbol;
        items.append(std::move(item));
    }
    return items;
}
