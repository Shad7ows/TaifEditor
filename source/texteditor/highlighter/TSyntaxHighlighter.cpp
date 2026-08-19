#include "TSyntaxHighlighter.h"

#include <QTextBlock>
#include <QTextDocument>

namespace {

TokenType baseTokenTypeForPresentation(const PresentationClass classification) {
    switch (classification) {
    case PresentationClass::Keyword: return TokenType::Keyword;
    case PresentationClass::Comment: return TokenType::Comment;
    case PresentationClass::String: return TokenType::String;
    case PresentationClass::Number: return TokenType::Number;
    case PresentationClass::Operator:
    case PresentationClass::Punctuation: return TokenType::Operator;
    case PresentationClass::FunctionDeclaration: return TokenType::Function;
    case PresentationClass::ClassDeclaration: return TokenType::ClassDef;
    case PresentationClass::SelfReceiver: return TokenType::Self;
    case PresentationClass::Builtin: return TokenType::BuiltinFunc;
    default: return TokenType::Identifier;
    }
}

} // namespace

TSyntaxHighlighter::TSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
    lexer = std::make_unique<TLexer>();
}

void TSyntaxHighlighter::setTheme(const std::shared_ptr<SyntaxTheme>& theme) {
    currentThemeFormats.clear();
    if (theme) {
        theme->apply(currentThemeFormats);
    }
    rehighlight();
}

void TSyntaxHighlighter::runFastPass(const quint64 revision, const DirtyRange& dirty) {
    m_fastPassRevision = revision;
    QTextDocument* textDocument = document();
    if (textDocument == nullptr) {
        return;
    }
    QTextBlock first = textDocument->findBlock(qMax<qsizetype>(0, dirty.beginOffset));
    QTextBlock last = textDocument->findBlock(qMax<qsizetype>(dirty.beginOffset, dirty.endOffset));
    if (!first.isValid()) {
        first = textDocument->begin();
    }
    if (!last.isValid()) {
        last = textDocument->lastBlock();
    }
    QTextBlock block = first;
    while (block.isValid()) {
        rehighlightBlock(block);
        if (block == last) {
            // One dependent block is required because the legacy lexer carries
            // multi-line state through QSyntaxHighlighter block state.
            if (block.next().isValid()) {
                rehighlightBlock(block.next());
            }
            break;
        }
        block = block.next();
    }
}

void TSyntaxHighlighter::setSemanticSnapshot(LanguageAnalysisSnapshotPtr snapshot) {
    if (!snapshot) {
        return;
    }
    m_semanticSnapshot = std::move(snapshot);
    m_semanticRevision = m_semanticSnapshot->revision;
    rebuildSemanticBlockIndex();
    rehighlight();
}

void TSyntaxHighlighter::clearSemanticSnapshot(const quint64 revision) {
    if (revision < m_semanticRevision) {
        return;
    }
    m_semanticSnapshot.reset();
    m_semanticSpansByBlock.clear();
    m_semanticRevision = revision;
    rehighlight();
}

void TSyntaxHighlighter::highlightBlock(const QString& text) {
    int startState = previousBlockState();
    if (startState == -1) {
        startState = StateMasks::Normal;
    }

    const QVector<TToken> tokens = lexer->tokenize(text, startState);
    for (const TToken& token : tokens) {
        if (currentThemeFormats.contains(token.type)) {
            setFormat(token.start, token.length, currentThemeFormats[token.type]);
        }
    }

    applySemanticSpans(text);
    setCurrentBlockState(lexer->getFinalState());
}

void TSyntaxHighlighter::rebuildSemanticBlockIndex() {
    m_semanticSpansByBlock.clear();
    if (!m_semanticSnapshot || document() == nullptr) {
        return;
    }

    QTextDocument* textDocument = document();
    for (const PresentationSpan& span : m_semanticSnapshot->spans) {
        if (span.range.begin.offset >= span.range.end.offset) {
            continue;
        }
        QTextBlock block = textDocument->findBlock(span.range.begin.offset);
        while (block.isValid() && block.position() < span.range.end.offset) {
            m_semanticSpansByBlock[block.blockNumber()].append(span);
            block = block.next();
        }
    }
}

void TSyntaxHighlighter::applySemanticSpans(const QString& text) {
    if (!m_semanticSnapshot) {
        return;
    }
    const QTextBlock block = currentBlock();
    const QVector<PresentationSpan> spans = m_semanticSpansByBlock.value(block.blockNumber());
    const qsizetype blockBegin = block.position();
    const qsizetype blockEnd = blockBegin + text.size();
    for (const PresentationSpan& span : spans) {
        const qsizetype begin = qMax(blockBegin, span.range.begin.offset);
        const qsizetype end = qMin(blockEnd, span.range.end.offset);
        if (begin >= end) {
            continue;
        }
        setFormat(begin - blockBegin, end - begin, formatForPresentation(span));
    }
}

QTextCharFormat TSyntaxHighlighter::formatForPresentation(const PresentationSpan& span) const {
    QTextCharFormat format = currentThemeFormats.value(
        baseTokenTypeForPresentation(span.classification),
        currentThemeFormats.value(TokenType::Identifier));
    switch (span.classification) {
    case PresentationClass::Parameter:
    case PresentationClass::Local:
        format.setFontItalic(false);
        break;
    case PresentationClass::UnresolvedName:
        format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        format.setUnderlineColor(QColor(240, 100, 100));
        break;
    case PresentationClass::DuplicateDeclaration:
        format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        format.setUnderlineColor(QColor(255, 180, 70));
        break;
    case PresentationClass::Error:
        format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        format.setUnderlineColor(QColor(240, 100, 100));
        break;
    default:
        break;
    }
    return format;
}
