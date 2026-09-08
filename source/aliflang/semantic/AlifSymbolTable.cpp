#include "AlifSymbolTable.h"

#include <QtCore/QSet>

#include <algorithm>
#include <limits>
#include <utility>

namespace {

bool containsOffset(const SourceRange& range, const qsizetype offset) {
    return range.begin.offset <= offset && offset < range.end.offset;
}

qsizetype rangeWidth(const SourceRange& range) {
    return qMax<qsizetype>(0, range.end.offset - range.begin.offset);
}

QString firstNameSegment(const QString& name) {
    const qsizetype separator = name.indexOf(QChar(u'.'));
    return separator >= 0 ? name.left(separator) : name;
}

bool isDocumentSymbolKind(const SymbolKind kind) {
    switch (kind) {
    case SymbolKind::Function:
    case SymbolKind::Class:
    case SymbolKind::ImportModule:
    case SymbolKind::ImportMember:
        return true;
    default:
        return false;
    }
}

} // namespace

class SymbolTableBuilderImpl final {
public:
    explicit SymbolTableBuilderImpl(const SymbolTableInput& input)
        : m_input(input),
          m_module(input.module),
          m_model(std::make_shared<SemanticModel>()) {
        m_model->m_documentRevision = input.documentRevision;
        m_model->m_scopes.reserve(m_module.nodes().size() / 3 + 2);
        m_model->m_symbols.reserve(m_module.nodes().size() / 2 + 16);
        m_model->m_references.reserve(m_module.nodes().size());
    }

    [[nodiscard]] std::shared_ptr<const SemanticModel> run() {
        if (m_module.rootId() == InvalidAstNodeId || m_module.nodes().isEmpty()) {
            addDiagnostic(QStringLiteral("يدل004"),
                          QStringLiteral("قدّم المحلل شجرة بناء جملة مجردة (AST) تفتقر إلى جذر وحدة."),
                          {}, SemanticDiagnosticSeverity::Warning);
            return m_model;
        }

        const AstNode& root = node(m_module.rootId());
        const ScopeId prelude = createScope(ScopeKind::Prelude, InvalidScopeId,
                                            InvalidAstNodeId, root.range);
        m_model->m_preludeScope = prelude;
        addPreludeSymbols(prelude, root.range);

        const ScopeId moduleScope = createScope(ScopeKind::Module, prelude,
                                                root.id, root.range);
        m_model->m_moduleScope = moduleScope;

        indexNode(root.id, moduleScope);
        resolveNode(root.id, moduleScope, ReferenceKind::Read);
        return m_model;
    }

private:
    const SymbolTableInput& m_input;
    const AstModule& m_module;
    std::shared_ptr<SemanticModel> m_model;
    QHash<AstNodeId, ScopeId> m_scopeByOwner;
    QSet<AstNodeId> m_indexedScopes;
    QSet<AstNodeId> m_resolvedScopes;
    QSet<AstNodeId> m_activeIndexNodes;
    QSet<AstNodeId> m_activeResolveNodes;
    bool m_suppressionReported = false;

    [[nodiscard]] const AstNode& node(const AstNodeId id) const {
        return m_module.node(id);
    }

    [[nodiscard]] bool isValidNode(const AstNodeId id) const {
        return id >= 0 && id < m_module.nodes().size();
    }

    [[nodiscard]] AstChildRole roleAt(const AstNode& ast, const qsizetype index) const {
        if (index >= 0 && index < ast.childRoles.size()) {
            return ast.childRoles.at(index);
        }
        return AstChildRole::Unknown;
    }

    [[nodiscard]] QVector<AstNodeId> childrenWithRole(const AstNode& ast,
                                                       const AstChildRole role) const {
        QVector<AstNodeId> result;
        for (qsizetype index = 0; index < ast.children.size(); ++index) {
            if (roleAt(ast, index) == role && isValidNode(ast.children.at(index))) {
                result.append(ast.children.at(index));
            }
        }
        return result;
    }

    [[nodiscard]] AstNodeId firstChildWithRole(const AstNode& ast,
                                                const AstChildRole role) const {
        const QVector<AstNodeId> matches = childrenWithRole(ast, role);
        return matches.isEmpty() ? InvalidAstNodeId : matches.constFirst();
    }

    void addDiagnostic(const QString& code, const QString& message,
                       const SourceRange& range,
                       const SemanticDiagnosticSeverity severity) {
        constexpr int maximumDetailedDiagnostics = 96;
        if (m_model->m_diagnostics.size() >= maximumDetailedDiagnostics) {
            if (!m_suppressionReported) {
                m_suppressionReported = true;
                m_model->m_diagnostics.append({
                    QStringLiteral("يدل999"),
                    QStringLiteral("تم تعطيل المزيد من عمليات التشخيص الدلالي للحفاظ على استجابة المحرر."),
                    range,
                    SemanticDiagnosticSeverity::Information
                });
            }
            return;
        }
        for (const SemanticDiagnostic& existing : std::as_const(m_model->m_diagnostics)) {
            if (existing.code == code && existing.message == message
                && existing.range.begin.offset == range.begin.offset
                && existing.range.end.offset == range.end.offset) {
                return;
            }
        }
        m_model->m_diagnostics.append({code, message, range, severity});
    }

    [[nodiscard]] ScopeId createScope(const ScopeKind kind, const ScopeId parent,
                                      const AstNodeId ownerNode,
                                      const SourceRange& range) {
        Scope scope;
        scope.id = m_model->m_scopes.size();
        scope.kind = kind;
        scope.parent = parent;
        scope.ownerNode = ownerNode;
        scope.range = range;
        m_model->m_scopes.append(std::move(scope));
        const ScopeId id = m_model->m_scopes.size() - 1;
        if (parent != InvalidScopeId && parent >= 0 && parent < m_model->m_scopes.size()) {
            m_model->m_scopes[parent].children.append(id);
        }
        if (ownerNode != InvalidAstNodeId) {
            m_scopeByOwner.insert(ownerNode, id);
        }
        return id;
    }

    void addPreludeSymbols(const ScopeId scope, const SourceRange& range) {
        static const QStringList builtins {
            QStringLiteral("اطبع"), QStringLiteral("ادخل"), QStringLiteral("مدى"),
            QStringLiteral("طول"), QStringLiteral("مصفوفة"), QStringLiteral("فهرس"),
            QStringLiteral("مميزة"), QStringLiteral("نص"), QStringLiteral("صحيح"),
            QStringLiteral("عشري"), QStringLiteral("هذا"), QStringLiteral("مترابطة"),
            QStringLiteral("تحقق_اي"), QStringLiteral("افتح"), QStringLiteral("هل_نوع"),
            QStringLiteral("مقرون"), QStringLiteral("معكوس"), QStringLiteral("منطق"),
            QStringLiteral("اصل"),
            // Built-in Alif runtime-error types. They are valid names in
            // exception clauses and must not be reported as unresolved.
            QStringLiteral("خطأ_اسم"), QStringLiteral("خطأ_مفتاح"),
            QStringLiteral("خطأ_ملف_غير_موجود"), QStringLiteral("خطأ_ليس_مجلد"),
            QStringLiteral("خطأ_صلاحيات"), QStringLiteral("خطأ_ترميز"),
            QStringLiteral("خطأ_غير_منهي"), QStringLiteral("خطأ_نظام_تشغيل"),
            QStringLiteral("خطأ_نوع"),
        };
        for (const QString& builtin : builtins) {
            declare(scope, SymbolKind::Builtin, builtin, range, range,
                    InvalidAstNodeId, true);
        }
    }

    [[nodiscard]] SymbolId declare(const ScopeId scopeId, const SymbolKind kind,
                                   const QString& name,
                                   const SourceRange& declarationRange,
                                   const SourceRange& fullRange,
                                   const AstNodeId declarationNode,
                                   const bool recoverable = false) {
        if (scopeId < 0 || scopeId >= m_model->m_scopes.size() || name.isEmpty()) {
            addDiagnostic(QStringLiteral("يدل004"),
                          QStringLiteral("لا يمكن التصريح عن رمز دون نطاق واسم صالحين"),
                          declarationRange, SemanticDiagnosticSeverity::Warning);
            return InvalidSymbolId;
        }

        Scope& scope = m_model->m_scopes[scopeId];
        const QVector<SymbolId> existing = scope.declarations.value(name);
        if (kind != SymbolKind::Builtin) {
            for (const SymbolId existingId : existing) {
                if (m_model->m_symbols.at(existingId).kind == kind) {
                    return existingId;
                }
            }
        }

        Symbol symbol;
        symbol.id = m_model->m_symbols.size();
        symbol.kind = kind;
        symbol.name = name;
        symbol.declarationRange = declarationRange;
        symbol.fullRange = fullRange;
        symbol.declarationNode = declarationNode;
        symbol.declaringScope = scopeId;
        symbol.isRecoverable = recoverable;
        m_model->m_symbols.append(std::move(symbol));
        const SymbolId id = m_model->m_symbols.size() - 1;
        scope.declarations[name].append(id);
        return id;
    }

    [[nodiscard]] ScopeId scopeForOwner(const AstNodeId ownerNode) const {
        return m_scopeByOwner.value(ownerNode, InvalidScopeId);
    }

    [[nodiscard]] ScopeId nearestClassScope(ScopeId scope) const {
        while (scope != InvalidScopeId && scope >= 0 && scope < m_model->m_scopes.size()) {
            if (m_model->m_scopes.at(scope).kind == ScopeKind::Class) {
                return scope;
            }
            scope = m_model->m_scopes.at(scope).parent;
        }
        return InvalidScopeId;
    }

    [[nodiscard]] SymbolId classSymbolForScope(const ScopeId classScope) const {
        for (auto it = m_model->m_classScopesBySymbol.cbegin();
             it != m_model->m_classScopesBySymbol.cend(); ++it) {
            if (it.value() == classScope) {
                return it.key();
            }
        }
        return InvalidSymbolId;
    }

    [[nodiscard]] ScopeId createOwnedScope(const ScopeKind kind, const ScopeId parent,
                                           const AstNode& owner) {
        const ScopeId existing = scopeForOwner(owner.id);
        if (existing != InvalidScopeId) {
            return existing;
        }
        return createScope(kind, parent, owner.id, owner.range);
    }

    void indexNode(const AstNodeId id, const ScopeId scope) {
        if (!isValidNode(id) || m_activeIndexNodes.contains(id)) {
            return;
        }
        m_activeIndexNodes.insert(id);
        const AstNode& ast = node(id);

        switch (ast.kind) {
        case AstNodeKind::FunctionDeclaration:
            indexFunction(ast, scope);
            break;
        case AstNodeKind::ClassDeclaration:
            indexClass(ast, scope);
            break;
        case AstNodeKind::ImportStatement:
            indexImport(ast, scope);
            break;
        case AstNodeKind::FromImportStatement:
            indexFromImport(ast, scope);
            break;
        case AstNodeKind::AssignmentStatement:
            indexAssignment(ast, scope);
            break;
        case AstNodeKind::ForStatement:
            indexFor(ast, scope);
            break;
        case AstNodeKind::ComprehensionExpression:
            indexComprehension(ast, scope);
            break;
        case AstNodeKind::LambdaExpression:
            indexLambda(ast, scope);
            break;
        default:
            for (const AstNodeId child : ast.children) {
                indexNode(child, scope);
            }
            break;
        }
        m_activeIndexNodes.remove(id);
    }

    void indexFunction(const AstNode& ast, const ScopeId enclosingScope) {
        const AstNodeId nameId = firstChildWithRole(ast, AstChildRole::DeclarationName);
        const AstNodeId parameterList = firstChildWithRole(ast, AstChildRole::ParameterList);
        const AstNodeId body = firstChildWithRole(ast, AstChildRole::Body);
        if (!isValidNode(nameId) || !isValidNode(body)) {
            addDiagnostic(QStringLiteral("يدل004"),
                          QStringLiteral("يفتقر تعريف الدالة إلى الأدوار الفرعية المطلوبة في شجرة بناء الجملة المجردة (AST)."),
                          ast.range, SemanticDiagnosticSeverity::Warning);
            return;
        }
        const AstNode& name = node(nameId);
        const SymbolId functionSymbol = declare(
            enclosingScope, SymbolKind::Function, ast.text.isEmpty() ? name.text : ast.text,
            name.range, ast.range, ast.id);
        const ScopeId functionScope = createOwnedScope(ScopeKind::Function, enclosingScope, ast);
        if (functionSymbol != InvalidSymbolId) {
            m_model->m_scopeOwnerSymbols.insert(functionScope, functionSymbol);
        }
        if (m_indexedScopes.contains(ast.id)) {
            return;
        }
        m_indexedScopes.insert(ast.id);
        if (isValidNode(parameterList)) {
            const AstNode& parameters = node(parameterList);
            for (const AstNodeId parameterId : parameters.children) {
                if (!isValidNode(parameterId)) {
                    continue;
                }
                const AstNode& parameter = node(parameterId);
                if (parameter.kind != AstNodeKind::Parameter) {
                    continue;
                }
                const AstNodeId parameterName = firstChildWithRole(parameter,
                                                                    AstChildRole::ParameterName);
                if (isValidNode(parameterName)) {
                    const AstNode& nameNode = node(parameterName);
                    const SymbolId parameterSymbol = declare(
                        functionScope, SymbolKind::Parameter, parameter.text.isEmpty()
                            ? nameNode.text : parameter.text, nameNode.range, parameter.range,
                        parameter.id);
                    const ScopeId classScope = nearestClassScope(enclosingScope);
                    if (parameterSymbol != InvalidSymbolId && nameNode.text == QStringLiteral("هذا")
                        && classScope != InvalidScopeId) {
                        m_model->m_symbols[parameterSymbol].instanceClass =
                            classSymbolForScope(classScope);
                    }
                } else {
                    addDiagnostic(QStringLiteral("يدل004"),
                                  QStringLiteral("المعامل يفتقر لاسم"), parameter.range,
                                  SemanticDiagnosticSeverity::Warning);
                }
            }
        }
        indexNode(body, functionScope);
    }

    void indexClass(const AstNode& ast, const ScopeId enclosingScope) {
        const AstNodeId nameId = firstChildWithRole(ast, AstChildRole::DeclarationName);
        const AstNodeId body = firstChildWithRole(ast, AstChildRole::Body);
        if (!isValidNode(nameId) || !isValidNode(body)) {
            addDiagnostic(QStringLiteral("يدل004"),
                          QStringLiteral("يفتقر تعريف الصنف إلى الأدوار الفرعية المطلوبة في شجرة بناء الجملة المجردة (AST)."),
                          ast.range, SemanticDiagnosticSeverity::Warning);
            return;
        }
        const AstNode& name = node(nameId);
        const SymbolId classSymbol = declare(
            enclosingScope, SymbolKind::Class, ast.text.isEmpty() ? name.text : ast.text,
            name.range, ast.range, ast.id);
        const ScopeId classScope = createOwnedScope(ScopeKind::Class, enclosingScope, ast);
        if (classSymbol != InvalidSymbolId) {
            m_model->m_classScopesBySymbol.insert(classSymbol, classScope);
            m_model->m_scopeOwnerSymbols.insert(classScope, classSymbol);
        }
        if (!m_indexedScopes.contains(ast.id)) {
            m_indexedScopes.insert(ast.id);
            indexNode(body, classScope);
        }
    }

    void indexImport(const AstNode& ast, const ScopeId scope) {
        const AstNodeId pathId = firstChildWithRole(ast, AstChildRole::ImportPath);
        if (!isValidNode(pathId)) {
            addDiagnostic(QStringLiteral("يدل004"),
                          QStringLiteral("يفتقر الاستيراد إلى مسار الاستيراد"),
                          ast.range, SemanticDiagnosticSeverity::Warning);
            return;
        }
        const AstNode& path = node(pathId);
        declare(scope, SymbolKind::ImportModule, firstNameSegment(path.text), path.range,
                ast.range, ast.id, true);
    }

    void indexFromImport(const AstNode& ast, const ScopeId scope) {
        const QVector<AstNodeId> names = childrenWithRole(ast, AstChildRole::ImportName);
        for (const AstNodeId nameId : names) {
            const AstNode& name = node(nameId);
            declare(scope, SymbolKind::ImportMember, name.text, name.range,
                    ast.range, ast.id, true);
        }
    }

    void indexAssignment(const AstNode& ast, const ScopeId scope) {
        if (ast.assignmentTargetCount < 0
            || ast.assignmentTargetCount >= ast.children.size()) {
            addDiagnostic(QStringLiteral("يدل004"),
                          QStringLiteral("الإسناد لا يملك هدف"),
                          ast.range, SemanticDiagnosticSeverity::Warning);
            return;
        }
        QVector<SymbolId> declaredTargets;
        for (qsizetype index = 0; index < ast.assignmentTargetCount; ++index) {
            if (roleAt(ast, index) != AstChildRole::Target || !isValidNode(ast.children.at(index))) {
                addDiagnostic(QStringLiteral("يدل004"),
                              QStringLiteral("هدف الإسناد لا يطابق هدف البيانات"),
                              ast.range, SemanticDiagnosticSeverity::Warning);
                continue;
            }
            const SymbolKind targetKind = m_model->m_scopes.at(scope).kind == ScopeKind::Class
                                              ? SymbolKind::Field : SymbolKind::Local;
            declaredTargets += declareTarget(ast.children.at(index), scope, targetKind);
        }

        const AstNodeId value = firstChildWithRole(ast, AstChildRole::Value);
        if (isValidNode(value)) {
            indexNode(value, scope);
        }
        const SymbolId instanceClass = isValidNode(value)
                                           ? directConstructorClass(node(value), scope) : InvalidSymbolId;
        if (instanceClass != InvalidSymbolId) {
            for (const SymbolId target : declaredTargets) {
                if (target >= 0 && target < m_model->m_symbols.size()) {
                    m_model->m_symbols[target].instanceClass = instanceClass;
                }
            }
        }
    }

    void indexFor(const AstNode& ast, const ScopeId scope) {
        const AstNodeId target = firstChildWithRole(ast, AstChildRole::Target);
        const AstNodeId body = firstChildWithRole(ast, AstChildRole::Body);
        if (isValidNode(target)) {
            declareTarget(target, scope, SymbolKind::LoopVariable);
        }
        if (isValidNode(body)) {
            indexNode(body, scope);
        }
    }

    void indexComprehension(const AstNode& ast, const ScopeId parentScope) {
        const ScopeId scope = createOwnedScope(ScopeKind::Comprehension, parentScope, ast);
        if (m_indexedScopes.contains(ast.id)) {
            return;
        }
        m_indexedScopes.insert(ast.id);
        const AstNodeId target = firstChildWithRole(ast, AstChildRole::Target);
        const AstNodeId iterable = firstChildWithRole(ast, AstChildRole::Iterable);
        const AstNodeId element = firstChildWithRole(ast, AstChildRole::Element);
        if (isValidNode(target)) {
            declareTarget(target, scope, SymbolKind::ComprehensionVariable);
        }
        if (isValidNode(iterable)) {
            indexNode(iterable, parentScope);
        }
        if (isValidNode(element)) {
            indexNode(element, scope);
        }
    }

    void indexLambda(const AstNode& ast, const ScopeId parentScope) {
        const ScopeId scope = createOwnedScope(ScopeKind::Lambda, parentScope, ast);
        if (m_indexedScopes.contains(ast.id)) {
            return;
        }
        m_indexedScopes.insert(ast.id);
        if (ast.children.size() < 2) {
            addDiagnostic(QStringLiteral("يدل005"),
                          QStringLiteral("أدوار معاملات الخطية ليست صريحة بعد في شجرة البنية المجردة"),
                          ast.range, SemanticDiagnosticSeverity::Information);
            return;
        }
        for (qsizetype index = 0; index + 1 < ast.children.size(); ++index) {
            const AstNodeId child = ast.children.at(index);
            if (!isValidNode(child)) {
                continue;
            }
            const AstNode& parameter = node(child);
            const AstNodeId parameterName = parameter.kind == AstNodeKind::Parameter
                                                ? firstChildWithRole(parameter, AstChildRole::ParameterName)
                                                : child;
            if (!isValidNode(parameterName)) {
                continue;
            }
            const AstNode& name = node(parameterName);
            declare(scope, SymbolKind::Parameter, name.text, name.range,
                    parameter.range, parameter.id, false);
        }

        for (qsizetype index = 0; index + 1 < ast.children.size(); ++index) {
            const AstNode& parameter = node(ast.children.at(index));
            if (parameter.kind == AstNodeKind::Parameter) {
                const AstNodeId defaultValue = firstChildWithRole(
                    parameter, AstChildRole::DefaultValue);
                if (isValidNode(defaultValue)) {
                    indexNode(defaultValue, parentScope);
                }
            }
        }
        indexNode(ast.children.constLast(), scope);
    }

    [[nodiscard]] SymbolId directConstructorClass(const AstNode& value,
                                                  const ScopeId scope) const {
        if (value.kind != AstNodeKind::CallExpression) {
            return InvalidSymbolId;
        }
        const AstNodeId calleeId = firstChildWithRole(value, AstChildRole::Callee);
        if (!isValidNode(calleeId) || node(calleeId).kind != AstNodeKind::NameExpression) {
            return InvalidSymbolId;
        }
        const QVector<SymbolId> candidates = lookup(scope, node(calleeId).text);
        if (candidates.size() != 1) {
            return InvalidSymbolId;
        }
        const SymbolId candidate = candidates.constFirst();
        return m_model->m_symbols.at(candidate).kind == SymbolKind::Class
                   ? candidate : InvalidSymbolId;
    }

    [[nodiscard]] QVector<SymbolId> declareTarget(const AstNodeId id, const ScopeId scope,
                                                  const SymbolKind kind) {
        if (!isValidNode(id)) {
            return{};
        }
        const AstNode& target = node(id);
        if (target.kind == AstNodeKind::NameExpression) {
            return {declare(scope, kind, target.text, target.range, target.range, target.id)};
        }
        if (target.kind == AstNodeKind::TupleExpression
            || target.kind == AstNodeKind::ListExpression) {
            QVector<SymbolId> targets;
            for (const AstNodeId child : target.children) {
                targets += declareTarget(child, scope, kind);
            }
            return targets;
        }
        if (target.kind == AstNodeKind::MemberExpression) {
            const AstNodeId base = firstChildWithRole(target, AstChildRole::MemberBase);
            const AstNodeId member = firstChildWithRole(target, AstChildRole::MemberName);
            const ScopeId classScope = nearestClassScope(scope);
            if (isValidNode(base) && isValidNode(member) && classScope != InvalidScopeId
                && node(base).kind == AstNodeKind::NameExpression
                && node(base).text == QStringLiteral("هذا")) {
                const AstNode& memberName = node(member);
                const QVector<SymbolId> existing =
                    m_model->m_scopes.at(classScope).declarations.value(memberName.text);
                for (const SymbolId existingId : existing) {
                    if (m_model->m_symbols.at(existingId).kind == SymbolKind::Field) {
                        return {existingId};
                    }
                }
                return {declare(classScope, SymbolKind::Field, memberName.text,
                                memberName.range, target.range, target.id)};
            }
        }
        addDiagnostic(QStringLiteral("يدل003"),
                      QStringLiteral("هدف إسناد غير صحيح"),
                      target.range, SemanticDiagnosticSeverity::Warning);
        return{};
    }

    void resolveCallArgument(const AstNode& ast, const ScopeId scope) {
        if (!ast.children.isEmpty() && isValidNode(ast.children.constFirst())) {
            resolveNode(ast.children.constFirst(), scope, ReferenceKind::Read);
        }
    }

    void resolveNode(const AstNodeId id, const ScopeId scope, const ReferenceKind context) {
        if (!isValidNode(id) || m_activeResolveNodes.contains(id)) {
            return;
        }
        m_activeResolveNodes.insert(id);
        const AstNode& ast = node(id);

        switch (ast.kind) {
        case AstNodeKind::FunctionDeclaration:
            resolveFunction(ast, scope);
            break;
        case AstNodeKind::ClassDeclaration:
            resolveClass(ast, scope);
            break;
        case AstNodeKind::AssignmentStatement:
            resolveAssignment(ast, scope);
            break;
        case AstNodeKind::ForStatement:
            resolveFor(ast, scope);
            break;
        case AstNodeKind::ComprehensionExpression:
            resolveComprehension(ast, scope);
            break;
        case AstNodeKind::LambdaExpression:
            resolveLambda(ast, scope);
            break;
        case AstNodeKind::NameExpression:
            addReference(ast, scope, context);
            break;
        case AstNodeKind::MemberExpression:
            resolveMember(ast, scope);
            break;
        case AstNodeKind::CallExpression:
            resolveCall(ast, scope);
            break;
        case AstNodeKind::KeywordArgument:
        case AstNodeKind::StarArgument:
        case AstNodeKind::DoubleStarArgument:
            resolveCallArgument(ast, scope);
            break;
        case AstNodeKind::DeleteStatement:
            for (const AstNodeId child : ast.children) {
                resolveNode(child, scope, ReferenceKind::Delete);
            }
            break;
        case AstNodeKind::ImportStatement:
        case AstNodeKind::FromImportStatement:
            break;
        case AstNodeKind::ErrorExpression:
        case AstNodeKind::ErrorStatement:
            for (const AstNodeId child : ast.children) {
                resolveNode(child, scope, context);
            }
            break;
        default:
            for (const AstNodeId child : ast.children) {
                resolveNode(child, scope, ReferenceKind::Read);
            }
            break;
        }
        m_activeResolveNodes.remove(id);
    }

    void resolveFunction(const AstNode& ast, const ScopeId enclosingScope) {
        const ScopeId functionScope = scopeForOwner(ast.id);
        if (functionScope == InvalidScopeId) {
            return;
        }
        const AstNodeId parameterList = firstChildWithRole(ast, AstChildRole::ParameterList);
        if (isValidNode(parameterList)) {
            for (const AstNodeId parameterId : node(parameterList).children) {
                if (!isValidNode(parameterId)) {
                    continue;
                }
                const AstNode& parameter = node(parameterId);
                const AstNodeId defaultValue = firstChildWithRole(parameter,
                                                                  AstChildRole::DefaultValue);
                if (isValidNode(defaultValue)) {
                    resolveNode(defaultValue, enclosingScope, ReferenceKind::Read);
                }
            }
        }
        if (!m_resolvedScopes.contains(ast.id)) {
            m_resolvedScopes.insert(ast.id);
            const AstNodeId body = firstChildWithRole(ast, AstChildRole::Body);
            if (isValidNode(body)) {
                resolveNode(body, functionScope, ReferenceKind::Read);
            }
        }
    }

    void resolveClass(const AstNode& ast, const ScopeId enclosingScope) {
        for (const AstNodeId base : childrenWithRole(ast, AstChildRole::Base)) {
            resolveNode(base, enclosingScope, ReferenceKind::BaseType);
        }
        const ScopeId classScope = scopeForOwner(ast.id);
        if (classScope != InvalidScopeId && !m_resolvedScopes.contains(ast.id)) {
            m_resolvedScopes.insert(ast.id);
            const AstNodeId body = firstChildWithRole(ast, AstChildRole::Body);
            if (isValidNode(body)) {
                resolveNode(body, classScope, ReferenceKind::Read);
            }
        }
    }

    void resolveAssignment(const AstNode& ast, const ScopeId scope) {
        if (ast.assignmentTargetCount < 0 || ast.children.size() < 1) {
            return;
        }
        for (qsizetype index = 0; index < ast.assignmentTargetCount; ++index) {
            if (isValidNode(ast.children.at(index))) {
                resolveTarget(ast.children.at(index), scope, ReferenceKind::Write);
            }
        }
        const AstNodeId value = firstChildWithRole(ast, AstChildRole::Value);
        if (isValidNode(value)) {
            resolveNode(value, scope, ReferenceKind::Read);
        }
    }

    void resolveFor(const AstNode& ast, const ScopeId scope) {
        const AstNodeId iterable = firstChildWithRole(ast, AstChildRole::Iterable);
        const AstNodeId target = firstChildWithRole(ast, AstChildRole::Target);
        const AstNodeId body = firstChildWithRole(ast, AstChildRole::Body);
        if (isValidNode(iterable)) {
            resolveNode(iterable, scope, ReferenceKind::Read);
        }
        if (isValidNode(target)) {
            resolveTarget(target, scope, ReferenceKind::Write);
        }
        if (isValidNode(body)) {
            resolveNode(body, scope, ReferenceKind::Read);
        }
    }

    void resolveComprehension(const AstNode& ast, const ScopeId parentScope) {
        const AstNodeId iterable = firstChildWithRole(ast, AstChildRole::Iterable);
        const AstNodeId target = firstChildWithRole(ast, AstChildRole::Target);
        const AstNodeId element = firstChildWithRole(ast, AstChildRole::Element);
        if (isValidNode(iterable)) {
            resolveNode(iterable, parentScope, ReferenceKind::Read);
        }
        const ScopeId scope = scopeForOwner(ast.id);
        if (scope == InvalidScopeId) {
            return;
        }
        if (isValidNode(target)) {
            resolveTarget(target, scope, ReferenceKind::Write);
        }
        if (isValidNode(element)) {
            resolveNode(element, scope, ReferenceKind::Read);
        }
    }

    void resolveLambda(const AstNode& ast, const ScopeId parentScope) {
        const ScopeId scope = scopeForOwner(ast.id);
        if (scope == InvalidScopeId || ast.children.isEmpty()) {
            return;
        }
        for (qsizetype index = 0; index + 1 < ast.children.size(); ++index) {
            const AstNode& parameter = node(ast.children.at(index));
            if (parameter.kind != AstNodeKind::Parameter) {
                continue;
            }
            const AstNodeId defaultValue = firstChildWithRole(
                parameter, AstChildRole::DefaultValue);
            if (isValidNode(defaultValue)) {
                resolveNode(defaultValue, parentScope, ReferenceKind::Read);
            }
        }
        resolveNode(ast.children.constLast(), scope, ReferenceKind::Read);
    }

    void resolveTarget(const AstNodeId id, const ScopeId scope, const ReferenceKind kind) {
        if (!isValidNode(id)) {
            return;
        }
        const AstNode& target = node(id);
        if (target.kind == AstNodeKind::NameExpression) {
            addReference(target, scope, kind);
            return;
        }
        if (target.kind == AstNodeKind::TupleExpression
            || target.kind == AstNodeKind::ListExpression) {
            for (const AstNodeId child : target.children) {
                resolveTarget(child, scope, kind);
            }
            return;
        }
        if (target.kind == AstNodeKind::MemberExpression) {
            resolveMember(target, scope);
            return;
        }
        addDiagnostic(QStringLiteral("يدل003"),
                      QStringLiteral("هدف إسناد غير صحيح"),
                      target.range, SemanticDiagnosticSeverity::Warning);
    }

    void resolveMember(const AstNode& ast, const ScopeId scope) {
        const AstNodeId base = firstChildWithRole(ast, AstChildRole::MemberBase);
        const AstNodeId member = firstChildWithRole(ast, AstChildRole::MemberName);
        if (isValidNode(base)) {
            resolveNode(base, scope, ReferenceKind::Read);
        }
        if (!isValidNode(member)) {
            return;
        }

        QVector<SymbolId> candidates;
        if (isValidNode(base) && node(base).kind == AstNodeKind::NameExpression) {
            const QVector<SymbolId> receivers = lookup(scope, node(base).text);
            if (receivers.size() == 1) {
                const QVector<SymbolId> members =
                    m_model->membersOfReceiver(receivers.constFirst());
                for (const SymbolId memberId : members) {
                    if (m_model->m_symbols.at(memberId).name == node(member).text) {
                        candidates.append(memberId);
                    }
                }
            }
        }
        addMemberReference(node(member), scope, candidates);
    }

    void resolveCall(const AstNode& ast, const ScopeId scope) {
        const AstNodeId callee = firstChildWithRole(ast, AstChildRole::Callee);
        if (isValidNode(callee)) {
            resolveNode(callee, scope, ReferenceKind::Call);
        }
        for (const AstNodeId argument : childrenWithRole(ast, AstChildRole::Argument)) {
            resolveNode(argument, scope, ReferenceKind::Read);
        }
    }

    void addReference(const AstNode& ast, const ScopeId scope, const ReferenceKind kind) {
        if (ast.text.isEmpty()) {
            addDiagnostic(QStringLiteral("يدل004"),
                          QStringLiteral("تعبير الاسم لا يحتوي على تهجئة مصدرية"), ast.range,
                          SemanticDiagnosticSeverity::Warning);
            return;
        }
        NameReference reference;
        reference.id = m_model->m_references.size();
        reference.node = ast.id;
        reference.name = ast.text;
        reference.range = ast.range;
        reference.containingScope = scope;
        reference.kind = kind;
        reference.candidates = lookup(scope, ast.text);
        if (reference.candidates.isEmpty()) {
            reference.state = ResolutionState::Unresolved;
            addDiagnostic(QStringLiteral("يدل001"),
                          QStringLiteral("اسم غير موجود '%1'").arg(ast.text), ast.range,
                          SemanticDiagnosticSeverity::Warning);
            return;
        }
        reference.state = ResolutionState::Resolved;
        reference.resolvedSymbol = reference.candidates.constFirst();
        appendReference(std::move(reference));
    }

    void addMemberReference(const AstNode& ast, const ScopeId scope,
                            QVector<SymbolId> candidates = {}) {
        NameReference reference;
        reference.id = m_model->m_references.size();
        reference.node = ast.id;
        reference.name = ast.text;
        reference.range = ast.range;
        reference.containingScope = scope;
        reference.kind = ReferenceKind::Member;
        reference.candidates = std::move(candidates);
        if (reference.candidates.size() == 1) {
            reference.state = ResolutionState::Resolved;
            reference.resolvedSymbol = reference.candidates.constFirst();
        } else if (reference.candidates.size() > 1) {
            reference.state = ResolutionState::Ambiguous;
        } else {
            // Unknown receivers/members remain external until type/module analysis.
            reference.state = ResolutionState::External;
        }
        appendReference(std::move(reference));
    }

    [[nodiscard]] QVector<SymbolId> lookup(ScopeId scope, const QString& name) const {
        while (scope != InvalidScopeId && scope >= 0 && scope < m_model->m_scopes.size()) {
            const Scope& current = m_model->m_scopes.at(scope);
            const QVector<SymbolId> matches = current.declarations.value(name);
            if (!matches.isEmpty()) {
                return matches;
            }
            scope = current.parent;
        }
        return {};
    }

    void appendReference(NameReference reference) {
        const ReferenceId id = reference.id;
        const SymbolId resolved = reference.resolvedSymbol;
        m_model->m_references.append(std::move(reference));
        if (resolved != InvalidSymbolId) {
            m_model->m_referencesBySymbol[resolved].append(id);
        }
    }
};

const Scope* SemanticModel::scope(const ScopeId id) const {
    return id >= 0 && id < m_scopes.size() ? &m_scopes.at(id) : nullptr;
}

const Symbol* SemanticModel::symbol(const SymbolId id) const {
    return id >= 0 && id < m_symbols.size() ? &m_symbols.at(id) : nullptr;
}

const NameReference* SemanticModel::reference(const ReferenceId id) const {
    return id >= 0 && id < m_references.size() ? &m_references.at(id) : nullptr;
}

const NameReference* SemanticModel::referenceAt(const qsizetype utf16Offset) const {
    const NameReference* best = nullptr;
    for (const NameReference& candidate : m_references) {
        if (!containsOffset(candidate.range, utf16Offset)) {
            continue;
        }
        if (best == nullptr || rangeWidth(candidate.range) < rangeWidth(best->range)) {
            best = &candidate;
        }
    }
    return best;
}

QVector<SymbolId> SemanticModel::visibleSymbolsAt(const qsizetype utf16Offset) const {
    ScopeId innermost = m_moduleScope;
    qsizetype narrowestWidth = std::numeric_limits<qsizetype>::max();
    for (const Scope& candidate : m_scopes) {
        if (candidate.kind == ScopeKind::Prelude || !containsOffset(candidate.range, utf16Offset)) {
            continue;
        }
        const qsizetype width = rangeWidth(candidate.range);
        if (width <= narrowestWidth) {
            innermost = candidate.id;
            narrowestWidth = width;
        }
    }

    QVector<SymbolId> result;
    QSet<QString> seenNames;
    ScopeId current = innermost;
    while (current != InvalidScopeId && current >= 0 && current < m_scopes.size()) {
        const Scope& scope = m_scopes.at(current);
        QStringList names = scope.declarations.keys();
        std::sort(names.begin(), names.end());
        for (const QString& name : names) {
            if (seenNames.contains(name)) {
                continue;
            }
            seenNames.insert(name);
            const QVector<SymbolId> declarations = scope.declarations.value(name);
            for (const SymbolId symbolId : declarations) {
                result.append(symbolId);
            }
        }
        current = scope.parent;
    }
    return result;
}

QVector<ReferenceId> SemanticModel::referencesOf(const SymbolId symbol) const {
    return m_referencesBySymbol.value(symbol);
}

QVector<SemanticBreadcrumb> SemanticModel::enclosingSymbolPathAt(
    const qsizetype utf16Offset) const {
    ScopeId innermost = InvalidScopeId;
    qsizetype narrowestWidth = std::numeric_limits<qsizetype>::max();
    for (const Scope& candidate : m_scopes) {
        if (candidate.kind == ScopeKind::Prelude || candidate.kind == ScopeKind::Module
            || !containsOffset(candidate.range, utf16Offset)) {
            continue;
        }
        const qsizetype width = rangeWidth(candidate.range);
        if (width <= narrowestWidth) {
            innermost = candidate.id;
            narrowestWidth = width;
        }
    }

    QVector<SemanticBreadcrumb> result;
    ScopeId current = innermost;
    while (current != InvalidScopeId && current >= 0 && current < m_scopes.size()) {
        const Scope& enclosingScope = m_scopes.at(current);
        const SymbolId ownerId = m_scopeOwnerSymbols.value(current, InvalidSymbolId);
        const Symbol* const owner = symbol(ownerId);
        if (owner != nullptr && (enclosingScope.kind == ScopeKind::Class
                                 || enclosingScope.kind == ScopeKind::Function)
            && containsOffset(owner->fullRange, utf16Offset)) {
            result.append({owner->id, owner->kind, owner->name,
                           owner->declarationRange, owner->fullRange});
        }
        current = enclosingScope.parent;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

QVector<SymbolId> SemanticModel::documentSymbols() const {
    QVector<SymbolId> result;
    for (const Symbol& candidate : m_symbols) {
        if (isDocumentSymbolKind(candidate.kind)) {
            result.append(candidate.id);
        }
    }
    std::sort(result.begin(), result.end(), [this](const SymbolId left, const SymbolId right) {
        return m_symbols.at(left).declarationRange.begin.offset
            < m_symbols.at(right).declarationRange.begin.offset;
    });
    return result;
}

SymbolId SemanticModel::classOfReceiver(const SymbolId receiverSymbol) const {
    const Symbol* receiver = symbol(receiverSymbol);
    if (receiver == nullptr) {
        return InvalidSymbolId;
    }
    if (receiver->kind == SymbolKind::Class) {
        return receiver->id;
    }
    return receiver->instanceClass;
}

QVector<SymbolId> SemanticModel::membersOfClass(const SymbolId classSymbol) const {
    const ScopeId classScope = m_classScopesBySymbol.value(classSymbol, InvalidScopeId);
    if (classScope == InvalidScopeId || classScope < 0 || classScope >= m_scopes.size()) {
        return {};
    }
    QVector<SymbolId> members;
    QStringList names = m_scopes.at(classScope).declarations.keys();
    std::sort(names.begin(), names.end());
    for (const QString& name : names) {
        for (const SymbolId id : m_scopes.at(classScope).declarations.value(name)) {
            const Symbol* candidate = symbol(id);
            if (candidate != nullptr && (candidate->kind == SymbolKind::Function
                                         || candidate->kind == SymbolKind::Field || candidate->kind == SymbolKind::Class)) {
                members.append(id);
            }
        }
    }
    return members;
}

QVector<SymbolId> SemanticModel::membersOfReceiver(const SymbolId receiverSymbol) const {
    const SymbolId classSymbol = classOfReceiver(receiverSymbol);
    return classSymbol == InvalidSymbolId ? QVector<SymbolId>{} : membersOfClass(classSymbol);
}

std::shared_ptr<const SemanticModel> SymbolTableBuilder::build(
    const SymbolTableInput& input,
    const std::shared_ptr<const SemanticModel>& previous) const {
    Q_UNUSED(previous)
    return SymbolTableBuilderImpl(input).run();
}
