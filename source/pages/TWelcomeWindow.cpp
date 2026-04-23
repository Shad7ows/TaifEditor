#include "TWelcomeWindow.h"
#include "Taif.h"
#include "TMenu.h"
#include <QtWidgets>


WelcomeWindow::WelcomeWindow(QWidget *parent)
    : QMainWindow(parent)
{

    setAttribute(Qt::WA_DeleteOnClose);

    setupStyle();

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainVLayout = new QVBoxLayout(centralWidget);
    this->setCentralWidget(centralWidget);

    QHBoxLayout *headerContent = new QHBoxLayout();
    QLabel *logoLabel = new QLabel();
    logoLabel->setPixmap(QPixmap(":/icons/resources/TaifLogo.ico").scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QVBoxLayout *textVLayout = new QVBoxLayout();
    QLabel *titleLabel = new QLabel("أهلا في محرر طيف");
    titleLabel->setObjectName("titleLabel");
    QFont titleFont;
    QStringList NotoKufiArabicFont = QFontDatabase::applicationFontFamilies(2);
    titleFont.setFamily(NotoKufiArabicFont.at(0));
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    QLabel *subtitleLabel = new QLabel("طيف - محرر لغة ألف");
    subtitleLabel->setObjectName("titleLabel");
    textVLayout->addWidget(titleLabel);
    textVLayout->addWidget(subtitleLabel);
    headerContent->addWidget(logoLabel);
    headerContent->addSpacing(15);
    headerContent->addSpacing(10);
    headerContent->addLayout(textVLayout);
    subtitleLabel->setFont(titleFont);

    QVBoxLayout *mainContentLayout = new QVBoxLayout();
    mainContentLayout->setSpacing(20);
    QSettings settings("Alif", "Taif");
    QStringList recentFiles = settings.value("RecentFiles").toStringList();
    QHBoxLayout *filesGroup = new QHBoxLayout();
    QVBoxLayout *filesButtons = new QVBoxLayout();
    newFileButton = new QPushButton("ملف جديد");
    newFileButton->setObjectName("primaryButton");
    openFileButton = new QPushButton("فتح الملف");
    openFolderButton = new QPushButton("فتح المجلد");
    filesButtons->addWidget(newFileButton);
    filesButtons->addWidget(openFileButton);
    filesButtons->addWidget(openFolderButton);
    filesButtons->addStretch();
    recentFilesList = new QListWidget();
    recentFilesList->addItems(recentFiles);
    recentFilesList->setMinimumWidth(450);
    recentFilesList->setWordWrap(true);
    filesGroup->addLayout(filesButtons);
    filesGroup->addWidget(recentFilesList);

    QHBoxLayout *sessionsGroup = new QHBoxLayout();
    QVBoxLayout *sessionsButtons = new QVBoxLayout();
    newSessionButton = new QPushButton("جلسة جديدة");
    manageSessionsButton = new QPushButton("إدارة الجلسات");
    sessionsButtons->addWidget(newSessionButton);
    sessionsButtons->addWidget(manageSessionsButton);
    sessionsButtons->addStretch();
    QLabel *noSessionsLabel = new QLabel("لا يوجد جلسات محفوظة");
    noSessionsLabel->setAlignment(Qt::AlignCenter);
    noSessionsLabel->setMinimumSize(450, 100);
    noSessionsLabel->setObjectName("sessionsBoxLabel");
    sessionsGroup->addLayout(sessionsButtons);
    sessionsGroup->addWidget(noSessionsLabel);

    int uniformWidth = 450;
    recentFilesList->setFixedWidth(uniformWidth);
    noSessionsLabel->setFixedWidth(uniformWidth);

    mainContentLayout->addLayout(filesGroup);
    mainContentLayout->addLayout(sessionsGroup);

    showOnStartupCheck = new QCheckBox("إظهار صفحة الترحيب عند بدأ البرنامج");
    showOnStartupCheck->setChecked(true);
    showOnStartupCheck->setDisabled(true);


    mainVLayout->addStretch(1);
    QHBoxLayout *headerCenteringLayout = new QHBoxLayout();
    headerCenteringLayout->addStretch();
    headerCenteringLayout->addLayout(headerContent);
    headerCenteringLayout->addStretch();
    mainVLayout->addLayout(headerCenteringLayout);
    mainVLayout->addSpacing(30);
    QHBoxLayout *contentCenteringLayout = new QHBoxLayout();
    contentCenteringLayout->addStretch();
    contentCenteringLayout->addLayout(mainContentLayout);
    contentCenteringLayout->addStretch();
    mainVLayout->addLayout(contentCenteringLayout);
    mainVLayout->addSpacing(20);
    // mainVLayout->addWidget(showOnStartupCheck, 0, Qt::AlignCenter);
    mainVLayout->addStretch(1);

    this->setWindowTitle("صفحة الترحيب - محرر طيف");
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeo = screen->availableGeometry();
    int margin = 100;
    int widthFixedNum = 6;
    int x = screenGeo.right() - screenGeo.size().width() + margin * widthFixedNum / 2;
    int y = screenGeo.top() + 30 + margin / 2; // 30 is top system bar height
    int width = screenGeo.size().width() - margin * widthFixedNum;
    int height = screenGeo.size().height() - margin;
    this->setGeometry(x, y, width, height);

    // ربط أزرار الواجهة المركزية
    connect(newFileButton, &QPushButton::clicked, this, &WelcomeWindow::handleNewFileRequest);
    connect(openFileButton, &QPushButton::clicked, this, &WelcomeWindow::handleOpenFileRequest);
    connect(openFolderButton, &QPushButton::clicked, this, &WelcomeWindow::handleOpenFolderRequest);
    connect(recentFilesList, &QListWidget::itemDoubleClicked, this, &WelcomeWindow::onRecentFileClicked);
}

void WelcomeWindow::setupStyle() {
    // Palette:
    // Background: #0f172a (Deep Navy)
    // Surface:    #1e293b (Slate Blue)
    // Accent:     #3b82f6 (Electric Blue)
    // Text:       #f1f5f9 (Off White)
    // Muted:      #94a3b8 (Slate Grey)

    QString styleSheet = R"(
        /* Main Window Container */
        QWidget {
            background-color: #0f172a;
            color: #f1f5f9;
            font-family: "Tajawal", Noto Kufi Arabic, Roboto, Helvetica, Arial, sans-serif;
        }

        /* General Labels */
        QLabel {
            color: #94a3b8;
            font-size: 13px;
        }
        /* The Title Label - Large and Bold */
        QLabel#titleLabel {
            color: #ffffff;
            font-size: 24px;
            font-weight: bold;
            margin-bottom: 10px;
        }


        /* --- Modern Buttons --- */
        QPushButton {
            min-width: 90px;
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            padding: 8px 16px;
            border-radius: 6px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #334155;
            border-color: #3b82f6;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #0f172a;
            color: #3b82f6;
        }

        /* Special Primary Button */
        QPushButton#primaryButton {
            background-color: #3b82f6;
            color: white;
            border: none;
        }
        QPushButton#primaryButton:hover {
            background-color: #2563eb;
        }

        /* --- Sessions List --- */
        QListWidget, QLabel#sessionsBoxLabel {
            background-color: #1e293b;
            border: 1px solid #334155;
            border-radius: 12px;
            color: #f1f5f9;
            outline: none;
            padding: 5px;
            font-size: 14px;
        }

        /* Styling the items inside the list to look like cards */
        QListWidget::item {
            background-color: transparent;
            color: #94a3b8;
            padding: 8px;
            border-radius: 8px;
            margin: 2px 0px;
        }
        QListWidget::item:hover {
            background-color: #334155;
            color: #ffffff;
        }
        QListWidget::item:selected {
            background-color: #3b82f6;
            color: #ffffff;
            border: 1px solid #60a5fa;
        }
    )";

    this->setStyleSheet(styleSheet);
}


void WelcomeWindow::onRecentFileClicked(QListWidgetItem *item)
{
    QString filePath = item->text();
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(this, "ملف غير موجود",
                             "تعذر العثور على الملف:\n" + filePath + "\n\nربما تم نقله أو حذفه.");
        // delete item;
        return;
    }

    Taif *editor = new Taif(filePath);
    editor->show();
    this->close();
}

void WelcomeWindow::handleNewFileRequest()
{
    Taif *editor = new Taif();
    editor->show();
    this->close();
}

void WelcomeWindow::handleOpenFileRequest()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open File");

    if (!filePath.isEmpty()) {
        Taif *editor = new Taif(filePath);
        editor->show();
        this->close();
    }
}

void WelcomeWindow::handleOpenFolderRequest()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "Open Folder");

    if (!folderPath.isEmpty()) {
        Taif *editor = new Taif();
        editor->loadFolder(folderPath);

        editor->show();
        this->close();
    }
}

void WelcomeWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
}

WelcomeWindow::~WelcomeWindow()
{
}
