#pragma once

#include <QPointer>
#include <QWidget>

class QCheckBox;
class QEvent;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

class SearchPanel final : public QWidget {
    Q_OBJECT

public:
    explicit SearchPanel(QWidget* parent = nullptr);

    [[nodiscard]] QString searchText() const;
    [[nodiscard]] QString replaceText() const;
    [[nodiscard]] bool isCaseSensitive() const;
    [[nodiscard]] bool isWholeWord() const;
    [[nodiscard]] bool isRegex() const;
    [[nodiscard]] QWidget* anchorWidget() const;

    /** Shows this persistent panel as a floating overlay at the anchor's upper RTL edge. */
    void showIn(QWidget* anchor);
    void showReplaceRow(bool visible);
    void setFocusToInput();
    void setMatchInfo(int currentMatch, int totalMatches);
    void setNoMatchesFound(bool noMatches);

signals:
    void findText();
    void findNext();
    void findPrevious();
    void replaceOne();
    void replaceAll();
    void closed();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateSearchInputAppearance();
    void repositionOverAnchor();

    QVBoxLayout* rootLayout = nullptr;
    QWidget* replaceRow = nullptr;
    QLineEdit* searchInput = nullptr;
    QLineEdit* replacementInput = nullptr;
    QPushButton* nextButton = nullptr;
    QPushButton* previousButton = nullptr;
    QPushButton* replaceButton = nullptr;
    QPushButton* replaceAllButton = nullptr;
    QPushButton* closeButton = nullptr;
    QCheckBox* caseCheckBox = nullptr;
    QCheckBox* wholeWordCheckBox = nullptr;
    QCheckBox* regexCheckBox = nullptr;
    QLabel* matchInfoLabel = nullptr;
    QPointer<QWidget> anchor{};
    bool noMatches = false;
};
