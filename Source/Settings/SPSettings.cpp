#include "SPSettings.h"


SPSettings::SPSettings(QWidget* parent) : QWidget(parent) {
    setWindowTitle("الإعدادات");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setMinimumSize(600, 400);
    setStyleSheet("color: #dddddd; background-color: #1e202e;");


    // Create UI elements
    QLabel* titleLabel = new QLabel("الإعدادات");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");

    // Create buttons
    QPushButton* saveButton = new QPushButton("حفظ");
    QPushButton* cancelButton = new QPushButton("إلغاء");

    // Layout setup
    QVBoxLayout* mainLayout = new QVBoxLayout();
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    // Connect signals
    connect(saveButton, &QPushButton::clicked, this, &SPSettings::saveSettings);
    connect(cancelButton, &QPushButton::clicked, this, &QWidget::close);

    loadSettings();
}


void SPSettings::closeEvent(QCloseEvent* event) {
    emit windowClosed();
    event->accept();
}


void SPSettings::loadSettings() {
    QSettings settings;
}

void SPSettings::saveSettings() {
    QSettings settings;
    emit settingsChanged();
    close();
}
