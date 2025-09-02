#pragma once

#include "THighlighter.h"
#include "AlifComplete.h"


class LineNumberArea;

class TEditor : public QPlainTextEdit {
	Q_OBJECT

public:
    TEditor(QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth() const;

    QString getCurrentLineIndentation(const QTextCursor &cursor) const;
    void curserIndentation();

public slots:
    void updateFontSize(int);

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
    LineNumberArea(TEditor* editor) : QWidget(editor), tEditor(editor) {
        this->setStyleSheet(
            "QWidget {"
            "   border-left: 1px solid #10a8f4;"
            "   border-top-left-radius: 9px;"        // Rounded top-left corner
            "   border-bottom-left-radius: 9px;"     // Rounded bottom-left corner
            "}"
        );
    }

    QSize sizeHint() const override {
        return QSize(tEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        tEditor->lineNumberAreaPaintEvent(event);
    }

private:
    TEditor* tEditor{};
};
