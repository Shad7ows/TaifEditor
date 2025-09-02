#include "AlifComplete.h"

#include <QVBoxLayout>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QStringList>
#include <QLabel>
#include <QTimer>
#include <QApplication>


AutoComplete::AutoComplete(QPlainTextEdit* editor, QObject* parent)
    : QObject(parent), editor(editor) {
    keywords = QStringList()
               << "اطبع"
               << "اذا"
               << "اواذا"
               << "استمر"
               << "ارجع"
               << "استورد"
               << "احذف"
               << "ادخل"
               << "اصل"
               << "او"
               << "انتظر"

               << "بينما"

               << "توقف"

               << "حاول"

               << "خطأ"
               << "خلل"

               << "دالة"

               << "صنف"
               << "صح"
               << "صحيح"

               << "عدم"
               << "عند"
               << "عام"
               << "عشري"

               << "في"

               << "ك"

               << "لاجل"
               << "ليس"

               << "مرر"
               << "من"
               << "مزامنة"
               << "مدى"
               << "مصفوفة"

               << "نطاق"
               << "نهاية"

               << "هل"

               << "والا"
               << "ولد"
               << "و"

               << "_تهيئة_";

    shortcuts = {
        {"اطبع", "اطبع($1)"},
        {"اذا", "اذا $1:\n\t\nوالا:\n\t"},
        {"اواذا", "اواذا $1:\n\t"},
        {"استمر", "استمر"},
        {"ارجع", "ارجع $1"},
        {"استورد", "استورد $1"},
        {"احذف", "احذف $1"},
        {"ادخل", "ادخل($1)"},
        {"اصل", "اصل()._تهيئة_($1)"},
        {"او", "او"},
        {"انتظر", "انتظر"},
        {"بينما", "بينما $1:\n\t"},
        {"توقف", "توقف"},
        {"حاول", "حاول:\n\t\nخلل:\n\t\nنهاية:\n\t"},
        {"خطأ", "خطأ"},
        {"خلل", "خلل:\n\t"},
        {"دالة", "دالة $1():\n\t"},
        {"صنف", "صنف $1:\n\tدالة _تهيئة_(هذا):\n\t\t"},
        {"صح", "صح"},
        {"صحيح", "صحيح($1)"},
        {"عدم", "عدم"},
        {"عند", "عند $1 ك :\n\t"},
        {"عام", "عام $1"},
        {"عشري", "عشري($1)"},
        {"في", "في"},
        {"ك", "ك"},
        {"لاجل", "لاجل $1 في :\n\t"},
        {"ليس", "ليس"},
        {"مرر", "مرر"},
        {"من", "من $1 استورد "},
        {"مزامنة", "مزامنة"},
        {"مدى", "مدى($1)"},
        {"مصفوفة", "مصفوفة($1)"},
        {"نطاق", "نطاق $1"},
        {"نهاية", "نهاية $1:\n\t"},
        {"هل", "هل"},
        {"والا", "والا:\n\t$1"},
        {"ولد", "ولد $1"},
        {"و", "و"},
        {"_تهيئة_", "دالة _تهيئة_(هذا):\n\t"}
    };        
    descriptions = {
        {"اطبع", "لعرض قيمة في الطرفية."},
        {"اذا", "تنفيذ أمر في حال تحقق الشرط."},
        {"اواذا", "التحقق من شرط إضافي بعد الشرط 'اذا'."},
        {"استمر", "الانتقال إلى التكرار التالي."},
        {"ارجع", "إرجاع قيمة من دالة."},
        {"استورد", "تضمين مكتبة خارجية."},
        {"احذف", "حذف متغير من الذاكرة."},
        {"ادخل", "قراءة مدخل من المستخدم."},
        {"اصل", "تستخدم لتهيئة الصنف الموروث."},
        {"او", "يكفي تحقق أحد الشرطين."},
        {"انتظر", "تتوقف الدالة عن التنفيذ الى حين قدوم النتائج."},
        {"بينما", "حلقة تعمل طالما أن الشرط صحيح."},
        {"توقف", "إيقاف تنفيذ تكرار الحلقة."},
        {"حاول", "محاولة تنفيذ الشفرة فإن ظهر خلل تنتقل إلى تنفيذ مرحلة'خلل'."},
        {"خطأ", "قيمة منطقية تدل على أن الشرط غير محقق."},
        {"خلل", "يتم تنفيذها في حال ظهور خلل ما في مرحلة تنفيذ 'حاول'."},
        {"دالة", "تعريف دالة جديدة تحتوي برنامج يتم تنفيذه عند استدعائها."},
        {"صنف", "إنشاء كائن يمتلك صفات ودوال."},
        {"صح", "قيمة منطقية تدل على أن الشرط محقق."},
        {"صحيح", "دالة ضمنية تقوم بتحويل المعامل الممرر الى عدد صحيح."},
        {"عدم", "قيمة فارغة."},
        {"عند", "تستخدم لفتح ملف خارجي والكتابة والقراءة عليه."},
        {"عام", "إخبار النطاق الداخلي أن هذا المتغير عام."},
        {"عشري", "دالة ضمنية تقوم بتحويل المعامل الممرر الى عدد عشري."},
        {"في", "تقوم بالتحقق ما إذا كانت القيمة ضمن حاوية مثل المصفوفة."},
        {"ك", "تحدد اسم الملف البديل عند فتحه."},
        {"لاجل", "حلقة تكرار ضمن مدى من الاعداد او مجموعة عناصر حاوية كالمصفوفة."},
        {"ليس", "نفي شرط أو قيمة."},
        {"مرر", "لا تقم بعمل شيء."},
        {"من", "تستخدم لاستيراد جزء محدد من ملف كاستيراد دالة واحدة."},
        {"مزامنة", "تجعل الدالة تزامنية بحيث تتوقف لإنتظار النتائج."},
        {"مدى", "تحديد مدى عددي من وإلى والخطوات."},
        {"مصفوفة", "دالة ضمنية تقوم بتحويل المعامل الممرر الى مصفوفة."},
        {"نطاق", "إخبار النطاق الداخلي أن هذا المتغير في نطاق اعلى ولكنه ليس عام."},
        {"نهاية", "يتم تنفيذ هذه الحالة بعد الإنتهاء من حالة 'حاول' مهما كانت النتيجة."},
        {"هل", "تستخدم للتحقق من قيمتين إن كانتا متطابقتين في النوع."},
        {"والا", "في حال عدم تحقق شرط 'اذا' يتم تنفيذها."},
        {"ولد", "تقوم بإرجاع قيم متتالية من دالة."},
        {"و", "أي يجب تحقق الشرطين معًا."},
        {"_تهيئة_", "دالة تقوم بتهيئة الصنف بشكل تلقائي عند استدعائه."},
    };    

    popup = new QWidget(editor, Qt::ToolTip | Qt::FramelessWindowHint);
    popup->setStyleSheet(
        "QWidget { background-color: #242533; color: #cccccc; }"
        "QListWidget { background-color: #242533; color: #cccccc; }"
        "QListWidget::item { padding: 7px; }"
        "QListWidget::item:selected { background-color: #3a3d54; color: #10a8f4; padding: 0 12px 0 0; }");

    QVBoxLayout* popupLayout = new QVBoxLayout(popup);
    popupLayout->setContentsMargins(0, 0, 0, 0);

    listWidget = new QListWidget(popup);

    QLabel* descriptionLabel = new QLabel(popup);
    descriptionLabel->setStyleSheet("color: #cccccc; padding: 3px;");
    descriptionLabel->setWordWrap(true);

    // set layouts
    popupLayout->addWidget(listWidget);
    popupLayout->addWidget(descriptionLabel);
    popup->setLayout(popupLayout);

    // connections
    connect(listWidget, &QListWidget::currentItemChanged, this,
            [=](QListWidgetItem* current, QListWidgetItem* previos) {
        if (!current) return;
        QString desc = descriptions.value(current->text(), QString());
        if (desc.isEmpty()) {
            return;
        }
        descriptionLabel->setText(desc);
    });
    connect(editor, &QPlainTextEdit::textChanged, this, &AutoComplete::showCompletion);
    connect(listWidget, &QListWidget::itemClicked, this, &AutoComplete::insertCompletion);

    // filters
    editor->installEventFilter(this);
}


bool AutoComplete::eventFilter(QObject* obj, QEvent* event) {
    if (obj == editor and event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (popup->isVisible()) {
            if (keyEvent->key() == Qt::Key_Tab
                or keyEvent->key() == Qt::Key_Return
                or keyEvent->key() == Qt::Key_Enter) {
                insertCompletion();
                return true;
            } else if (keyEvent->key() == Qt::Key_Escape) {
                hidePopup();
                return true;
            } else if (keyEvent->key() == Qt::Key_Up
                       or keyEvent->key() == Qt::Key_Down) {
                QCoreApplication::sendEvent(listWidget, event);
                return true;
            } else {
                return false;
            }
        }
    } else if (event->type() == QEvent::FocusOut) { // review
        QTimer::singleShot(0, this, [this]() {
            QWidget* newFocus = QApplication::focusWidget();
            if (!newFocus || !popup->isAncestorOf(newFocus)) {
                popup->hide();
            }
        });
    }
    return QObject::eventFilter(obj, event);
}

QString AutoComplete::getCurrentWord() const {
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    return cursor.selectedText().trimmed();
}

void AutoComplete::showCompletion() {
    QString currentWord = getCurrentWord();
    if (currentWord.isEmpty() or currentWord.length() < 1) {
        hidePopup();
        return;
    }

    QStringList suggestions{};
    for (const QString& keyword : keywords) {
        if (keyword.startsWith(currentWord, Qt::CaseInsensitive)) {
            suggestions << keyword;
        }
    }

    if (!suggestions.isEmpty()) {
        listWidget->clear();
        listWidget->addItems(suggestions);
        listWidget->setCurrentRow(0);
        showPopup();
    }
    else {
        hidePopup();
    }
}

void AutoComplete::showPopup() {
    QTextCursor cursor = editor->textCursor();
    QRect rect = editor->cursorRect(cursor);
    QPoint pos = editor->viewport()->mapToGlobal(rect.bottomLeft());

    // Set the minimum width of the popup
    popup->setFixedSize(450, 250);
    int popupWidth = popup->width();
    int popupHeight = popup->height();

    // Adjust the position to the left bottom of the cursor
    pos.setX(pos.x() - popup->width());
    pos.setY(pos.y() + 2); // Add a small offset to avoid overlapping

    // Ensure the popup stays within the screen boundaries
    QScreen* screen = QGuiApplication::screenAt(pos);
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();

        // Adjust X position if the popup goes off the right edge of the screen
        if (pos.x() + popupWidth > screenGeometry.right()) {
            pos.setX(screenGeometry.right() - popupWidth);
        }

        // Adjust Y position if the popup goes off the bottom edge of the screen
        if (pos.y() + popupHeight > screenGeometry.bottom()) {
            pos.setY(screenGeometry.bottom() - popupHeight);
        }
    }

    // Set the popup size and position
    popup->move(pos);
    popup->show();
}

inline void AutoComplete::hidePopup() {
    popup->hide();
}

void AutoComplete::insertCompletion() {
    if (!popup->isVisible()) return;

    QListWidgetItem* item = listWidget->currentItem();
    if (!item) return;

    QString word = item->text();
    if (!shortcuts.contains(word)) return;

    QString text = shortcuts.value(word);
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    placeholderPositions.clear();

    // البحث عن جميع العلامات مثل $1 وغيرها
    QRegularExpression re("\\$(\\d+)");
    QRegularExpressionMatchIterator i = re.globalMatch(text);
    QList<QPair<int, int>> matches;

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        int pos = match.capturedStart();
        int length = match.capturedLength();
        matches.append(qMakePair(pos, length));
    }

    // إزالة العلامات وحساب المواقع الجديدة
    QString newText = text;
    int offset = 0;
    for (const auto &match : matches) {
        int originalPos = match.first - offset;
        int length = match.second;
        newText.remove(originalPos, length);
        placeholderPositions.append(originalPos);
        offset += length;
    }

    cursor.insertText(newText);

    // حفظ المواقع وتحديد المؤشر
    if (!placeholderPositions.isEmpty()) {
        cursor.setPosition(cursor.position() - newText.length() + placeholderPositions.first());
        editor->setTextCursor(cursor);
    } else {
        editor->setTextCursor(cursor);
    }

    hidePopup();
}



bool AutoComplete::isPopupVisible() {
    return popup->isVisible();
}
