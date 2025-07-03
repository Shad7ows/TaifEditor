#pragma once

#include "SPHighlighter.h"
#include "AlifComplete.h"


class LineNumberArea;

class SPEditor : public QPlainTextEdit {
	Q_OBJECT

public:
	SPEditor(QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth() const;

    QString getCurrentLineIndentation(const QTextCursor &cursor) const;
    void curserIndentation();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;

private:
    SyntaxHighlighter* highlighter{};
    AutoComplete* autoComplete{};
    LineNumberArea* lineNumberArea{};

private slots:
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    inline void updateLineNumberArea(const QRect &rect, int dy);

signals:
    void openRequest(QString filePath);
};


class LineNumberArea : public QWidget {
public:
    LineNumberArea(SPEditor* editor) : QWidget(editor), spEditor(editor) {
        this->setStyleSheet(
            "QWidget {"
            "   border-left: 1px solid #10a8f4;"
            "   border-top-left-radius: 9px;"        // Rounded top-left corner
            "   border-bottom-left-radius: 9px;"     // Rounded bottom-left corner
            "}"
        );


#if defined(Q_OS_WIN)
        QString fontName = "Kawkab-Mono";
#elif defined(Q_OS_LINUX) or defined(Q_OS_MAC)
        QString fontName = "Kawkab Mono";
#endif

        this->setFont(QFont(fontName, 9));

    }

    QSize sizeHint() const override {
        return QSize(spEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        spEditor->lineNumberAreaPaintEvent(event);
    }

private:
    SPEditor* spEditor;
};
