#include "TSettings.h"


TSettings::TSettings(QWidget* parent) : QWidget(parent) {
    setWindowTitle("الإعدادات");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setMinimumSize(800, 600);
    setStyleSheet("color: #dddddd; background-color: #1e202e;");

    // Layout setup
    QHBoxLayout* mainLayout = new QHBoxLayout();
    QVBoxLayout* mainPropertyLayout = new QVBoxLayout();
    optionsLayout = new QVBoxLayout();
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    stackedWidget = new QStackedWidget();

    createCategory("المحرر", "إعدادات مظهر المحرر");
    // createCategory("متقدم", "الإعداد المتقدمة");


    optionsLayout->setAlignment(Qt::AlignTop);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(0);

    QWidget* optionsWidget = new QWidget();
    optionsWidget->setStyleSheet(".QWidget { border-left-width: 3px; border-left-style: ridge; border-left-color: #1e202f; }");
    optionsWidget->setLayout(optionsLayout);
    optionsWidget->setMinimumWidth(200);
    optionsWidget->setMaximumWidth(300);

    QWidget* propertyWidget = new QWidget();
    propertyWidget->setLayout(mainPropertyLayout);
    propertyWidget->setMinimumWidth(400);

    mainLayout->addWidget(optionsWidget);
    mainLayout->addWidget(stackedWidget);

    mainPropertyLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}


void TSettings::closeEvent(QCloseEvent* event) {
    QSettings settings("Alif", "Taif");
    settings.setValue("editorFontSize", fontSpin->value());

    // emit windowClosed();
    // event->accept();
}


void TSettings::switchPage() {
    TFlatButton* btn = qobject_cast<TFlatButton*>(sender());
    if (btn) {
        int index = btn->property("pageIndex").toInt();
        stackedWidget->setCurrentIndex(index);

        // Update button states
        for (TFlatButton* category : categories) {
            bool active = (category == btn);
            category->setStyleSheet(active ?
                                        "font-weight: bold; color: #10a8f4;" :
                                        "");
        }
    }
}

void TSettings::createCategory(const QString& name, const QString& description) {
    // Create and configure button
    TFlatButton* btn = new TFlatButton(this, name);
    btn->setProperty("pageIndex", categories.size());
    connect(btn, &QPushButton::clicked, this, &TSettings::switchPage);

    // Add button to left panel
    optionsLayout->addWidget(btn);

    // Create settings page
    QWidget* page = new QWidget;
    QVBoxLayout* pageLayout = new QVBoxLayout(page);
    pageLayout->setAlignment(Qt::AlignTop);

    // Add description label
    QLabel* descLabel = new QLabel(description);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #888; margin-bottom: 20px;");
    pageLayout->addWidget(descLabel);

    // Add category-specific content
    if (name == "المحرر") {
        createAppearancePage(pageLayout);
    } else if (name == "متقدم") {
        // createFontsPage(pageLayout);
    }

    // Add page to stacked widget
    stackedWidget->addWidget(page);
    categories.append(btn);
}

void TSettings::createAppearancePage(QVBoxLayout* layout) {
    // Font selection
    QGroupBox* fontGroup = new QGroupBox("الخط");
    fontGroup->setStyleSheet("QGroupBox { border: 1px solid gray; border-radius: 6px; margin-top: 2.0ex;}"
                             " QGroupBox::title { subcontrol-origin: margin; padding: 0 2px; left: 10px; }");
    QVBoxLayout* fontLayout = new QVBoxLayout(fontGroup);
    QFormLayout* fontSizeLayout = new QFormLayout();
    QFormLayout* fontFamilyLayout = new QFormLayout();

    fontSpin = new QSpinBox;
    fontSpin->setRange(12, 36);
    fontSpin->setMinimumHeight(40);
    fontSpin->setMaximumWidth(80);

    QSettings settingsVal("Alif", "Taif");
    int savedSize = settingsVal.value("editorFontSize").toInt();
    savedSize ? fontSpin->setValue(savedSize) : fontSpin->setValue(16);

    fontSizeLayout->addRow("حجم الخط: ", fontSpin);
    connect(fontSpin, &QSpinBox::valueChanged, this, &TSettings::fontSizeChanged);


    QComboBox* fontCombo = new QComboBox();
    fontCombo->setEditable(true);
    fontCombo->setInsertPolicy(QComboBox::NoInsert);
    fontCombo->setMinimumHeight(40);
    fontCombo->setMaximumWidth(200);

    QStringList fontFamilies = QFontDatabase::families();
    fontFamilies.sort(Qt::CaseInsensitive);
    // Add fonts to combobox
    foreach (const QString &family, fontFamilies) {
        fontCombo->addItem(family);
    }
    fontCombo->setCurrentText("Arial");

    fontFamilyLayout->addRow("نوع الخط: ", fontCombo);

    fontLayout->addLayout(fontSizeLayout);
    // fontLayout->addLayout(fontFamilyLayout);

    layout->addWidget(fontGroup);
}
