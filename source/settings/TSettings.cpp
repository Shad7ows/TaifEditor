#include "TSettings.h"

TSettings::TSettings(QWidget* parent) : QWidget(parent) {
    setWindowTitle("الإعدادات");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setMinimumSize(800, 600);
    setupStyling();

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
    optionsWidget->setObjectName("optionsWidget");
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

// TODO: Fix Active Buttons
void TSettings::setupStyling() {
    // Theme Palette
    // Background: #0f172a | Surface: #1e293b
    // Accent: #3b82f6 | Text: #f1f5f9 | Muted: #94a3b8

    QString mainStyle = R"(
        /* --- Main Dialog Background --- */
        QWidget {
            background-color: #0f172a;
            color: #f1f5f9;
            font-family: "Tajawal", Noto Kufi Arabic, Helvetica, Arial, sans-serif;
        }

        /* --- Sidebar / Options Container --- */
        QWidget#optionsWidget {
            background-color: #0f172a;
            border-left: 3px solid #1e293b;
        }

        /* --- Category Labels/Buttons --- */
        QLabel#categoryLabel {
            color: #94a3b8;
            font-size: 14px;
            padding: 8px;
            background: transparent;
        }
        /* This class will be toggled via C++ */
        QLabel#categoryLabel[active="true"] {
            color: #3b82f6;
            font-weight: bold;
            background-color: #1e293b;
            border-radius: 4px;
        }

        /* Description Text */
        QLabel#descLabel {
            color: #64748b;
            font-size: 16px;
            margin-bottom: 20px;
        }

        /* Modern GroupBoxes */
        QGroupBox {
            background-color: #0f172a;
            border: 1px solid #334155;
            border-radius: 8px;
            margin-top: 2.0ex;
            font-weight: bold;
            color: #f1f5f9;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 0 5px;
            color: #f1f5f9;
        }

        /* --- Inputs inside GroupBoxes --- */
        QLineEdit, QComboBox, QSpinBox {
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 5px 10px;
            background-color: #0f172a;
            selection-background-color: #339af0;
            color: #f1f5f9;
            font-size: 13px;
        }

        /* Hover effect */
        QLineEdit:hover, QComboBox:hover, QSpinBox:hover {
            border: 1px solid #339af0;
        }

        /* Focus effect (Modern blue outline) */
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus {
            border: 1px solid #339af0;
        }

        /* --- QComboBox Specifics --- */
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 25px;
            border-left-width: 0px; /* Removes the internal line */
            border-top-right-radius: 6px;
            border-bottom-right-radius: 6px;
        }

        QComboBox::down-arrow {
            image: url(:/icons/resources/chevron-down.svg);
            width: 18px;
            height: 18px;
        }

        /* The dropdown menu list */
        QComboBox QAbstractItemView {
            border: 1px solid #339af0;
            border-radius: 6px;
            background-color: #0f172a;
            selection-background-color: #1e293b;
            selection-color: #1c7ed6;
            outline: none;
        }

        /* QSpinBox Specifics */
        QSpinBox::up-button, QSpinBox::down-button {
            width: 20px;
            border-radius: 3px;
            background-color: #0f172a;
        }

        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #1e293b;
        }

        QSpinBox::up-arrow {
            image: url(:/icons/resources/chevron-up.svg);
            width: 16px;
        }
        QSpinBox::down-arrow {
            image: url(:/icons/resources/chevron-down.svg);
            width: 16px;
        }
    )";

    this->setStyleSheet(mainStyle);
}

void TSettings::setCategoryActive(TFlatButton* category, bool active) {
    category->setProperty("active", active);

    // IMPORTANT: In Qt, you must polish the widget to refresh the style
    // after changing a dynamic property
    category->style()->unpolish(category);
    category->style()->polish(category);
    category->update();
}

void TSettings::closeEvent(QCloseEvent* event) {
    QSettings settings("Alif", "Taif");
    settings.setValue("editorFontSize", fontSpin->value());
    settings.setValue("editorFontType", fontCombo->currentText());
    settings.setValue("editorCodeTheme", themeCombo->currentIndex());
    settings.sync();

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
            category->setObjectName("categoryLabel");
            bool active = (category == btn);
            setCategoryActive(category, active);
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
    // descLabel->setStyleSheet("color: #888; margin-bottom: 20px;");
    descLabel->setObjectName("descLabel");
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

    // ================== Font selection ==================
    QGroupBox* fontGroup = new QGroupBox("الخط");
    fontGroup->setObjectName("fontGroup");
    QVBoxLayout* fontLayout = new QVBoxLayout(fontGroup);
    QFormLayout* fontSizeLayout = new QFormLayout();
    QFormLayout* fontFamilyLayout = new QFormLayout();

    fontSpin = new QSpinBox;
    fontSpin->setRange(12, 36);
    fontSpin->setMinimumHeight(40);
    fontSpin->setMaximumWidth(80);

    QSettings settingsVal("Alif", "Taif");
    int savedSize = settingsVal.value("editorFontSize").toInt();
    savedSize ? fontSpin->setValue(savedSize) : fontSpin->setValue(18);

    fontSizeLayout->addRow("حجم الخط: ", fontSpin);
    connect(fontSpin, &QSpinBox::valueChanged, this, &TSettings::fontSizeChanged);


    fontCombo = new QComboBox();
    fontCombo->setEditable(true);
    fontCombo->setInsertPolicy(QComboBox::NoInsert);
    fontCombo->setMinimumHeight(40);
    fontCombo->setMaximumWidth(200);

    QStringList fontFamilies{};
    for (int i = 0; i < 3; i++) {
        QStringList font = QFontDatabase::applicationFontFamilies(i);
        fontFamilies.append(font.at(0));
    }

    // QStringList fontFamilies = QFontDatabase::families();
    // fontFamilies.sort(Qt::CaseInsensitive);
    // Add fonts to combobox
    foreach (const QString &family, fontFamilies) {
        fontCombo->addItem(family);
    }
    QString savedFont = settingsVal.value("editorFontType").toString();
    !savedFont.isEmpty() ? fontCombo->setCurrentText(savedFont) : fontCombo->setCurrentText("Noto Kufi Arabic");

    fontFamilyLayout->addRow("نوع الخط: ", fontCombo);
    connect(fontCombo, &QComboBox::currentTextChanged, this, &TSettings::fontTypeChanged);

    fontLayout->addLayout(fontSizeLayout);
    fontLayout->addLayout(fontFamilyLayout);

    // ================== Themes ==================
    QGroupBox* themeGroup = new QGroupBox("المظهر");
    themeGroup->setObjectName("themeGroup");
    QVBoxLayout* themeLayout = new QVBoxLayout(themeGroup);
    QFormLayout* comboLayout = new QFormLayout();

    themeCombo = new QComboBox();
    themeCombo->setInsertPolicy(QComboBox::NoInsert);
    themeCombo->setMinimumHeight(40);
    themeCombo->setMaximumWidth(250);

    setThemes();
    // Populate UI
    for (const auto& theme : availableThemes) {
        themeCombo->addItem(theme->name());
    }

    int savedTheme = settingsVal.value("editorCodeTheme").toInt();
    savedTheme ? themeCombo->setCurrentIndex(savedTheme) : themeCombo->setCurrentIndex(0);

    comboLayout->addRow("مظهر الشيفرة: ", themeCombo);
    connect(themeCombo, &QComboBox::currentIndexChanged, this, &TSettings::highlighterThemeChanged);

    themeLayout->addLayout(comboLayout);




    layout->addWidget(fontGroup);
    layout->addWidget(themeGroup);
}

QComboBox *TSettings::getThemeCombo() const {
    return themeCombo;
}

void TSettings::setThemes() {
    availableThemes.append(std::make_shared<VSCodeDarkTheme>());
    availableThemes.append(std::make_shared<MonokaiTheme>());
    availableThemes.append(std::make_shared<OceanicTheme>());
    availableThemes.append(std::make_shared<TaifGlowTheme>());
}

QVector<std::shared_ptr<SyntaxTheme>> TSettings::getAvailableThemes() const {
    return availableThemes;
}
