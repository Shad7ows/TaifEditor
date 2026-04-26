#include "TSettings.h"

TSettings::TSettings(QWidget* parent) : QWidget(parent) {
    setWindowTitle("الإعدادات");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setMinimumSize(800, 600);
    setLayoutDirection(Qt::RightToLeft);

    setupLayout();
    setupStyling();
}

void TSettings::setupLayout() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    sidebar = new QListWidget();
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(260);
    sidebar->setSpacing(5);
    sidebar->setFrameShape(QFrame::NoFrame);

    // Content Area
    stackedWidget = new QStackedWidget();
    stackedWidget->setObjectName("contentArea");

    // Connect Sidebar to StackedWidget
    connect(sidebar, &QListWidget::currentRowChanged, stackedWidget, &QStackedWidget::setCurrentIndex);

    // Build Pages
    addSettingPage("المحرر", "إعدادات مظهر المحرر", ":/icons/resources/pencil-ruler.svg");
    addSettingPage("متقدم", "خيارات النظام المتقدمة", ":/icons/resources/settings.svg");

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(stackedWidget);

    // Default to first page
    sidebar->setCurrentRow(0);
}

void TSettings::addSettingPage(const QString& name, const QString& description, const QString& iconPath) {
    // Create List Item
    QListWidgetItem* item = new QListWidgetItem(sidebar);
    item->setText(name);
    item->setIcon(QIcon(iconPath));
    item->setSizeHint(QSize(0, 50));
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Create Page
    QWidget* page = new QWidget();
    QVBoxLayout* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(30, 30, 30, 30);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel* descLabel = new QLabel(description);
    descLabel->setObjectName("descLabel");
    pageLayout->addWidget(descLabel);

    if (name == "المحرر") {
        createAppearancePage(pageLayout);
    }

    stackedWidget->addWidget(page);
}

void TSettings::setupStyling() {
    QString style = R"(
        /*  Theme Palette
            Background: #0f172a | Surface: #1e293b
            Accent: #3b82f6 | Text: #f1f5f9 | Muted: #94a3b8
        */

        /* Main Container */
        QWidget {
            background-color: #0f172a;
            color: #f1f5f9;
            font-family: "Tajawal", "Noto Kufi Arabic", "Segoe UI", sans-serif;
        }

        /* Sidebar Styling */
        QListWidget#sidebar {
            background-color: #1e293b;
            border-left: 3px double #334155;
            padding-top: 10px;
            outline: none;
            font-size: 18px;
        }

        /* The "Buttons" (List Items) */
        QListWidget#sidebar::item {
            color: #94a3b8;
            padding-right: 5px;
            border-right: 4px solid transparent;
            margin: 2px 0px;
        }

        QListWidget#sidebar::item:hover {
            background-color: #334155;
            color: #f1f5f9;
            border-radius: 8px;
        }

        /* The Active/Selected State */
        QListWidget#sidebar::item:selected {
            color: #3b82f6;
            font-weight: bold;
            border-right: 4px solid #3b82f6;
            border-radius: 0px;
        }

        /* Content Area Background */
        QStackedWidget#contentArea {
            background-color: #0f172a;
        }

        QLabel#descLabel {
            color: #64748b;
            font-size: 21px;
            margin-bottom: 15px;
        }

        /* Group Boxes */
        QGroupBox {
            background-color: #1e293b;
            border: 1px solid #334155;
            border-radius: 8px;
            margin-top: 25px;
            padding-top: 15px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 20px;
            color: #3b82f6;
        }

        /* --- Inputs --- */
        QGroupBox QLabel {
            background-color: #1e293b;
        }

        QLineEdit, QComboBox, QSpinBox{
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 5px 10px;
            background-color: #1e293b;
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
            background-color: #1e293b;
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 25px;
            border-left-width: 0px; /* Removes the internal line */
            border-radius: 6px;
        }

        QComboBox::down-arrow {
            image: url(:/icons/resources/chevron-down.svg);
            border-radius: 6px;
            width: 18px;
            height: 18px;
        }

        /* The dropdown menu list */
        QComboBox QAbstractItemView {
            border: 1px solid #339af0;
            border-radius: 6px;
            background-color: #1e293b;
            selection-background-color: #0f172a;
            selection-color: #1c7ed6;
            outline: none;
        }

        /* QSpinBox Specifics */
        QSpinBox::up-button, QSpinBox::down-button {
            width: 20px;
            border-radius: 6px;
            background-color: #1e293b;
        }

        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #1c7ed6;
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

    this->setStyleSheet(style);
}

void TSettings::closeEvent(QCloseEvent* event) {
    QSettings settings("Alif", "Taif");
    settings.setValue("editorFontSize", fontSpin->value());
    settings.setValue("editorFontType", fontCombo->currentText());
    settings.setValue("editorCodeTheme", themeCombo->currentIndex());
    settings.sync();

    event->accept();
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
    layout->setSpacing(30);
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
