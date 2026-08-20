#include "TSearchPanel.h"

#include <QCheckBox>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

constexpr int kPanelWidth = 540;
constexpr int kFindOnlyHeight = 48;
constexpr int kReplaceHeight = 96;
constexpr int kAnchorMargin = 12;

constexpr auto kPanelStyle = R"(
    QWidget#SearchPanel {
        background-color: #0f1b33;
        border: 1px solid #35577c;
        border-radius: 10px;
        font-family: "Tajawal", "Noto Kufi Arabic";
    }
    QLineEdit {
        background-color: #091427;
        color: #f1f5f9;
        border: 1px solid #385879;
        border-radius: 6px;
        padding: 6px 9px;
        selection-background-color: #2563eb;
    }
    QLineEdit:focus { border-color: #60a5fa; }
    QPushButton {
        background-color: transparent;
        color: #cbd5e1;
        border: 1px solid transparent;
        border-radius: 6px;
        padding: 6px 9px;
    }
    QPushButton:hover { background-color: #1e3a5f; color: #f8fafc; }
    QPushButton:pressed { background-color: #2563eb; color: #ffffff; }
    QPushButton:disabled { color: #64748b; }
    QCheckBox { color: #cbd5e1; spacing: 4px; }
    QCheckBox::indicator {
        width: 14px;
        height: 14px;
        border: 1px solid #476a90;
        border-radius: 3px;
        background-color: #091427;
    }
    QCheckBox::indicator:checked {
        border-color: #60a5fa;
        background-color: #2563eb;
    }
    QLabel { color: #93c5fd; min-width: 50px; }
)";

} // namespace

SearchPanel::SearchPanel(QWidget* const parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("SearchPanel"));
    setLayoutDirection(Qt::RightToLeft);
    setAttribute(Qt::WA_StyledBackground, true);
    setWindowFlags(Qt::FramelessWindowHint);
    setStyleSheet(QString::fromLatin1(kPanelStyle));
    setFixedWidth(kPanelWidth);

    auto* const shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 155));
    setGraphicsEffect(shadow);

    rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(9, 6, 9, 6);
    rootLayout->setSpacing(5);

    auto* const findRow = new QWidget(this);
    auto* const findLayout = new QHBoxLayout(findRow);
    findLayout->setContentsMargins(0, 0, 0, 0);
    findLayout->setSpacing(4);

    closeButton = new QPushButton(QStringLiteral("×"), findRow);
    closeButton->setObjectName(QStringLiteral("CloseSearchButton"));
    closeButton->setToolTip(QStringLiteral("إغلاق البحث"));
    closeButton->setFixedWidth(30);

    searchInput = new QLineEdit(findRow);
    searchInput->setObjectName(QStringLiteral("SearchInput"));
    searchInput->setPlaceholderText(QStringLiteral("بحث..."));
    searchInput->setClearButtonEnabled(true);

    nextButton = new QPushButton(QStringLiteral("التالي"), findRow);
    nextButton->setObjectName(QStringLiteral("FindNextButton"));
    previousButton = new QPushButton(QStringLiteral("السابق"), findRow);
    previousButton->setObjectName(QStringLiteral("FindPreviousButton"));

    caseCheckBox = new QCheckBox(QStringLiteral("Aa"), findRow);
    caseCheckBox->setObjectName(QStringLiteral("CaseSensitiveCheckBox"));
    caseCheckBox->setToolTip(QStringLiteral("مطابقة حالة الأحرف"));
    wholeWordCheckBox = new QCheckBox(QStringLiteral("كلمة"), findRow);
    wholeWordCheckBox->setObjectName(QStringLiteral("WholeWordCheckBox"));
    wholeWordCheckBox->setToolTip(QStringLiteral("مطابقة الكلمة كاملة"));
    regexCheckBox = new QCheckBox(QStringLiteral(".*"), findRow);
    regexCheckBox->setObjectName(QStringLiteral("RegexCheckBox"));
    regexCheckBox->setToolTip(QStringLiteral("استخدام تعبير نمطي"));

    matchInfoLabel = new QLabel(findRow);
    matchInfoLabel->setObjectName(QStringLiteral("SearchMatchInfo"));
    matchInfoLabel->setAlignment(Qt::AlignCenter);

    findLayout->addWidget(closeButton);
    findLayout->addWidget(searchInput, 1);
    findLayout->addWidget(nextButton);
    findLayout->addWidget(previousButton);
    findLayout->addWidget(caseCheckBox);
    findLayout->addWidget(wholeWordCheckBox);
    findLayout->addWidget(regexCheckBox);
    findLayout->addWidget(matchInfoLabel);
    rootLayout->addWidget(findRow);

    replaceRow = new QWidget(this);
    replaceRow->setObjectName(QStringLiteral("ReplaceRow"));
    auto* const replaceLayout = new QHBoxLayout(replaceRow);
    replaceLayout->setContentsMargins(0, 0, 0, 0);
    replaceLayout->setSpacing(4);

    replacementInput = new QLineEdit(replaceRow);
    replacementInput->setObjectName(QStringLiteral("ReplacementInput"));
    replacementInput->setPlaceholderText(QStringLiteral("استبدال بـ..."));
    replacementInput->setClearButtonEnabled(true);

    replaceButton = new QPushButton(QStringLiteral("استبدال"), replaceRow);
    replaceButton->setObjectName(QStringLiteral("ReplaceOneButton"));
    replaceAllButton = new QPushButton(QStringLiteral("استبدال الكل"), replaceRow);
    replaceAllButton->setObjectName(QStringLiteral("ReplaceAllButton"));

    replaceLayout->addWidget(replacementInput, 1);
    replaceLayout->addWidget(replaceButton);
    replaceLayout->addWidget(replaceAllButton);
    rootLayout->addWidget(replaceRow);

    connect(closeButton, &QPushButton::clicked, this, &SearchPanel::closed);
    connect(nextButton, &QPushButton::clicked, this, &SearchPanel::findNext);
    connect(previousButton, &QPushButton::clicked, this, &SearchPanel::findPrevious);
    connect(replaceButton, &QPushButton::clicked, this, &SearchPanel::replaceOne);
    connect(replaceAllButton, &QPushButton::clicked, this, &SearchPanel::replaceAll);
    connect(searchInput, &QLineEdit::returnPressed, this, &SearchPanel::findNext);
    connect(replacementInput, &QLineEdit::returnPressed, this, &SearchPanel::replaceOne);
    connect(searchInput, &QLineEdit::textChanged, this, [this]() {
        noMatches = false;
        updateSearchInputAppearance();
        emit findText();
    });

    const auto refreshSearch = [this]() { emit findText(); };
    connect(caseCheckBox, &QCheckBox::toggled, this, refreshSearch);
    connect(wholeWordCheckBox, &QCheckBox::toggled, this, refreshSearch);
    connect(regexCheckBox, &QCheckBox::toggled, this, refreshSearch);

    showReplaceRow(false);
    setMatchInfo(0, 0);
}

QString SearchPanel::searchText() const
{
    return searchInput->text();
}

QString SearchPanel::replaceText() const
{
    return replacementInput->text();
}

bool SearchPanel::isCaseSensitive() const
{
    return caseCheckBox->isChecked();
}

bool SearchPanel::isWholeWord() const
{
    return wholeWordCheckBox->isChecked();
}

bool SearchPanel::isRegex() const
{
    return regexCheckBox->isChecked();
}

QWidget* SearchPanel::anchorWidget() const
{
    return anchor.data();
}

void SearchPanel::showIn(QWidget* const target)
{
    if (target == nullptr) {
        hide();
        return;
    }

    if (anchor != target) {
        if (anchor != nullptr) {
            anchor->removeEventFilter(this);
        }
        anchor = target;
        anchor->installEventFilter(this);
    }

    repositionOverAnchor();
    show();
    raise();
    setFocusToInput();
}

void SearchPanel::showReplaceRow(const bool visible)
{
    replaceRow->setVisible(visible);
    setFixedHeight(visible ? kReplaceHeight : kFindOnlyHeight);
    repositionOverAnchor();
}

void SearchPanel::setFocusToInput()
{
    searchInput->setFocus(Qt::OtherFocusReason);
    searchInput->selectAll();
}

void SearchPanel::setMatchInfo(const int currentMatch, const int totalMatches)
{
    if (totalMatches > 0) {
        matchInfoLabel->setText(QStringLiteral("%1/%2").arg(currentMatch).arg(totalMatches));
    } else {
        matchInfoLabel->clear();
    }
}

void SearchPanel::setNoMatchesFound(const bool value)
{
    noMatches = value;
    updateSearchInputAppearance();
}

bool SearchPanel::eventFilter(QObject* const watched, QEvent* const event)
{
    if (watched == anchor) {
        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Move:
        case QEvent::Show:
            repositionOverAnchor();
            break;
        case QEvent::Hide:
            hide();
            break;
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SearchPanel::updateSearchInputAppearance()
{
    searchInput->setStyleSheet(noMatches
        ? QStringLiteral("border-color: #ef4444;")
        : QString());
}

void SearchPanel::repositionOverAnchor()
{
    if (anchor == nullptr || parentWidget() == nullptr) {
        return;
    }

    const QRect anchorRect = anchor->rect();
    const int x = qMax(kAnchorMargin, anchorRect.right() - width() - kAnchorMargin + 1);
    const QPoint anchorPosition = anchor->mapTo(parentWidget(), QPoint(x, kAnchorMargin));
    move(anchorPosition);
}
