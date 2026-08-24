#pragma once

#include <QTimer>
#include <QPoint>
#include <QScrollBar>
#include <QPlainTextEdit>
#include <QCompleter>
#include <memory>

#include "TSettings.h"
#include "EditorPreferences.h"
#include "EditorInfoSnapshot.h"
#include "AiAgentTypes.h"

class RecoveryCoordinator;
class EditorAnalysisBinding;
class EditorRecoveryBinding;
class EditorInteractionBinding;
class MultiCursorController;
struct RecoveryEntry;
struct RecoveryWriteResult;

#include "TSyntaxHighlighter.h"
#include "AutoComplete.h"
#include "AutoCompleteUI.h"
#include "EditorAnalysisController.h"
#include "SemanticCompletionProvider.h"
#include "SemanticHoverProvider.h"
#include "SemanticDefinitionProvider.h"
#include "CompletionContext.h"
#include "BreadcrumbTypes.h"


class LineNumberArea;
class TMinimap;
class THoverPopup;
class QEvent;
class QMouseEvent;
class QFrame;
class QLabel;
class QPushButton;

class TEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    TEditor(TSettings* setting = nullptr, QWidget* parent = nullptr);
    ~TEditor() override;

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth() const;
    QString filePath{};

    QString getCurrentLineIndentation(const QTextCursor &cursor) const;
    void curserIndentation();

    void setCompleter(QCompleter *completer);

    void startAutoSave();
    void stopAutoSave();
    void removeBackupFile();
    void setRecoveryCoordinator(RecoveryCoordinator* coordinator);
    [[nodiscard]] QString recoveryDocumentId() const;
    [[nodiscard]] bool hasPendingRecoveryPersistence() const;
    [[nodiscard]] bool isRecoveryRetryScheduled() const;
    [[nodiscard]] quint64 lastRequestedRecoveryRevision() const;
    [[nodiscard]] quint64 lastPersistedRecoveryRevision() const;
    [[nodiscard]] quint64 currentDirtyRecoveryRevision() const;
    void flushRecoverySnapshot();
    void adoptRecoveryEntry(const RecoveryEntry& entry);

public:
    [[nodiscard]] const QVector<EditorDiagnostic>& currentDiagnostics() const {
        return m_currentDiagnostics;
    }
    [[nodiscard]] EditorBreadcrumbContext breadcrumbContextAtCursor() const;
    [[nodiscard]] EditorInfoSnapshot informationSnapshot() const;
    void setDocumentLineEnding(EditorInfoSnapshot::LineEnding lineEnding);
    [[nodiscard]] int secondaryCursorCount() const;
    [[nodiscard]] int totalCursorCount() const;
    void clearSecondaryCursors();

    /**
     * Renders a targeted AI edit inside this editor as a temporary red/green
     * projection. The file is never written until final approval.
     */
    void presentAiInlinePatch(const AiPatchReviewRequest& review);
    void clearAiInlinePatch();
    [[nodiscard]] QString aiInlineReviewId() const;

public slots:
    void UpdateTabStopDistance(QFont);
    void updateFontSize(int);
    void updateFontType(QString font);
    void toggleComment();
    void duplicateLine();
    void moveLineUp();
    void moveLineDown();
    void performAutoSave();
    void updateHighlighterTheme(std::shared_ptr<SyntaxTheme>);
    void applyPreferences(const EditorPreferences& preferences);
    void navigateToDiagnosticRange(SourceRange range);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    // We override focusOutEvent to close the popup if the user clicks away
    void focusOutEvent(QFocusEvent *e) override;

    void paintEvent(QPaintEvent *event) override;

private:
        TSyntaxHighlighter* highlighter{};
    EditorAnalysisBinding* analysisBinding{};
    // Non-owning convenience reference; EditorAnalysisBinding owns the controller.
    EditorAnalysisController* analysisController{};

    std::unique_ptr<SemanticCompletionProvider> semanticCompletionProvider{};
    SemanticHoverProvider semanticHoverProvider{};
    SemanticDefinitionProvider semanticDefinitionProvider{};
    THoverPopup* hoverPopup{};
    EditorInteractionBinding* interactionBinding{};
    std::unique_ptr<MultiCursorController> multiCursorController{};
    bool multiCursorTransactionInProgress = false;

    // Ephemeral AI review state. Preview source is restored verbatim on reject;
    // no streamed content may be saved or sent to recovery storage.
    bool m_aiInlineActive = false;
    bool m_aiInlineFinal = false;
    bool m_aiInlineWasReadOnly = false;
    int m_aiInlineFocusedLine = -1;
    QString m_aiInlineReviewId;
    QString m_aiInlineOriginalText;
    struct AiInlineHunk final {
        int startLine = 0;
        int endLine = 0;
        QString replacement;
    };
    QVector<AiInlineHunk> m_aiInlineHunks;
    QVector<QTextEdit::ExtraSelection> m_aiInlineRemovedSelections;
    QVector<QTextEdit::ExtraSelection> m_aiInlineAddedSelections;
    QTimer m_aiInlineRenderTimer;
    std::optional<AiPatchReviewRequest> m_pendingAiInlineReview;
    QFrame* m_aiInlineReviewBar = nullptr;
    QLabel* m_aiInlineReviewLabel = nullptr;
    QPushButton* m_aiInlineApplyButton = nullptr;
    QPushButton* m_aiInlineRejectButton = nullptr;
    std::optional<SourceRange> ctrlHoverDefinitionRange{};

    struct NavigationHistoryEntry {
        qsizetype anchor = 0;
        qsizetype position = 0;
    };
    QVector<NavigationHistoryEntry> definitionNavigationHistory{};
    QVector<EditorDiagnostic> m_currentDiagnostics{};
    quint64 m_diagnosticsRevision = 0;

    LineNumberArea* lineNumberArea{};
    TMinimap* minimap{};

    struct FoldRegion {
        int startBlockNumber;
        int endBlockNumber;
        bool folded = false;
    };
    QVector<FoldRegion> foldRegions;

        void updateFoldRegions();
    void toggleFold(int blockNum);
    void scheduleRecoveryCapture();
    void clearRecoverySnapshot();


    EditorRecoveryBinding* recoveryBinding{};

    EditorPreferences preferences{};
    EditorInfoSnapshot::LineEnding documentLineEnding = EditorInfoSnapshot::LineEnding::Unknown;

    friend class LineNumberArea;
    friend class TMinimap;

    QCompleter* c{};
    CompletionModel *model{};
    std::vector<std::unique_ptr<ICompletionStrategy>> strategies{};
    QStringList snippetTargets{};
    CompletionContext activeCompletionContext{};
    quint64 activeCompletionRevision = 0;
    bool hasActiveCompletionContext = false;
    // Set only by a user-triggered completion pass while semantic analysis is pending.
    bool completionRefreshPending = false;
    [[nodiscard]] bool canRefreshActiveCompletion() const;
    void clearActiveCompletionContext();
    void dismissCompletionPopup();
    bool handleMultiCursorKeyPress(QKeyEvent* event);
    void applyMultiCursorNewline();
    void paintSecondaryCursors(QPainter& painter);
    bool rebuildAiInlineProjection(const AiPatchReviewRequest& review);
    void updateAiInlineSelections();
    void updateAiInlineReviewBar();
    void positionAiInlineReviewBar();
    void applyAiInlinePatch(const AiPatchReviewRequest& review);
    void clearSecondaryCursorsForSingleCursorAction();
    void scheduleHover(const QPoint& viewportPosition);
    void showPendingHover();
    void dismissHover();
    void updateCtrlHoverDefinitionLink(const QPoint& viewportPosition);
    void clearCtrlHoverDefinitionLink();
    [[nodiscard]] std::optional<DefinitionLocation> definitionAt(qsizetype offset) const;
    bool navigateToDefinition(qsizetype offset);
    bool navigateBackFromDefinition();
    void notifyBreadcrumbContextChanged();
    void notifyEditorInformationChanged();
    QTextCursor textUnderCursor() const;
    void performCompletion();
    bool processSnippetNavigation();
    void setupAutoComplete();
    void insertWord(const QString& completion, QTextCursor& tc);
    void insertBuiltinFunction(const QString& functionName, QTextCursor& tc);
    void insertSnippet(const QString& snippet, QTextCursor& tc);
    // Bracket auto-completion methods
    bool handleAutoPairing(QKeyEvent* e);
    bool handleBracketCompletion(QChar openingBracket, QChar closingBracket);
    bool handleQuoteCompletion(QChar quoteChar);
    bool handleBracketSkip(QChar typedChar);

private slots:
    void updateMinimapPosition();
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    inline void updateLineNumberArea(const QRect &rect, int dy);
    void insertCompletion(const QString &completion, CompletionType type);
signals:
    void openRequest(QString filePath);
    void diagnosticsChanged(QVector<EditorDiagnostic> diagnostics, quint64 revision);
    void breadcrumbContextChanged(EditorBreadcrumbContext context);
    void editorInformationChanged(EditorInfoSnapshot snapshot);
    void aiInlinePatchAccepted(QString reviewId);
    void aiInlinePatchRejected(QString reviewId);
};


class LineNumberArea : public QWidget {
public:
    LineNumberArea(TEditor* editor) : QWidget(editor), tEditor(editor) {
        this->setStyleSheet(
            "QWidget {"
            "   border-left: 1px solid #10a8f4;"
            "   border-top-left-radius: 9px;"
            "   border-bottom-left-radius: 9px;"
            "}"
        );
    }

    QSize sizeHint() const override {
        return QSize(tEditor->lineNumberAreaWidth(), 0);
    }

    void mousePressEvent(QMouseEvent* event) override {
        int y = event->position().y();
        QTextBlock block = tEditor->firstVisibleBlock();
        int top = qRound(tEditor->blockBoundingGeometry(block).translated(tEditor->contentOffset()).top());
        int height = qRound(tEditor->blockBoundingRect(block).height());

        while (block.isValid() && top <= y) {
            if (y >= top && y < top + height) {
                int blockNum = block.blockNumber();
                tEditor->toggleFold(blockNum);
                return;
            }
            block = block.next();
            top += height;
            height = qRound(tEditor->blockBoundingRect(block).height());
        }
    }


protected:
    void paintEvent(QPaintEvent* event) override {
        tEditor->lineNumberAreaPaintEvent(event);
    }


private:
    TEditor* tEditor{};
};
