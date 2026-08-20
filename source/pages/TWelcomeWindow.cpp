#include "TWelcomeWindow.h"
#include "ApplicationBootstrap.h"

#include "SessionEditorDialog.h"
#include "Taif.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QStackedLayout>
#include <QVBoxLayout>

#include <optional>
#include <utility>

namespace {

constexpr int kSessionIdRole = Qt::UserRole;

QListWidgetItem* addSessionItem(QListWidget* const list, const SavedSession& session)
{
    const QString fileCount = QString::number(session.filePaths.size());
    auto* const item = new QListWidgetItem(
        QStringLiteral("%1\n%2 ملف").arg(session.displayName, fileCount), list);
    item->setData(kSessionIdRole, session.id);
    item->setToolTip(session.filePaths.isEmpty()
        ? QStringLiteral("جلسة فارغة")
        : session.filePaths.join(u'\n'));
    return item;
}

const SavedSession* sessionForItem(const QVector<SavedSession>& sessions,
                                   const QListWidgetItem* const item)
{
    if (item == nullptr) {
        return nullptr;
    }
    const QString id = item->data(kSessionIdRole).toString();
    for (const SavedSession& session : sessions) {
        if (session.id == id) {
            return &session;
        }
    }
    return nullptr;
}

} // namespace

WelcomeWindow::WelcomeWindow(QWidget* const parent,
                             SessionStore::SettingsScope sessionScope)
    : QMainWindow(parent)
    , sessionStore(std::move(sessionScope))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setLayoutDirection(Qt::RightToLeft);
    setupStyle();

    auto* const centralWidget = new QWidget(this);
    auto* const mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    auto* const headerContent = new QHBoxLayout();
    auto* const logoLabel = new QLabel(centralWidget);
    logoLabel->setPixmap(QPixmap(QStringLiteral(":/icons/resources/TaifLogo.ico"))
                             .scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* const textLayout = new QVBoxLayout();
    auto* const titleLabel = new QLabel(QStringLiteral("أهلاً بك في محرر طيف"), centralWidget);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    auto* const subtitleLabel = new QLabel(QStringLiteral("طيف — محرر لغة ألف"), centralWidget);

    QFont titleFont = titleLabel->font();
    const QString displayArabicFamily = ApplicationBootstrap::displayArabicFamily();
    if (!displayArabicFamily.isEmpty()) {
        titleFont.setFamily(displayArabicFamily);
    }
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    subtitleLabel->setFont(titleFont);
    textLayout->addWidget(titleLabel);
    textLayout->addWidget(subtitleLabel);
    headerContent->addWidget(logoLabel);
    headerContent->addSpacing(15);
    headerContent->addLayout(textLayout);

    auto* const mainContentLayout = new QVBoxLayout();
    mainContentLayout->setSpacing(20);

    auto* const filesGroup = new QHBoxLayout();
    auto* const filesButtons = new QVBoxLayout();
    newFileButton = new QPushButton(QStringLiteral("ملف جديد"), centralWidget);
    newFileButton->setObjectName(QStringLiteral("primaryButton"));
    openFileButton = new QPushButton(QStringLiteral("فتح ملف"), centralWidget);
    openFolderButton = new QPushButton(QStringLiteral("فتح مجلد"), centralWidget);
    filesButtons->addWidget(newFileButton);
    filesButtons->addWidget(openFileButton);
    filesButtons->addWidget(openFolderButton);
    filesButtons->addStretch();

    recentFilesList = new QListWidget(centralWidget);
    recentFilesList->setObjectName(QStringLiteral("RecentFilesList"));
    QSettings settings(QStringLiteral("Alif"), QStringLiteral("Taif"));
    recentFilesList->addItems(settings.value(QStringLiteral("RecentFiles")).toStringList());
    recentFilesList->setWordWrap(true);
    recentFilesList->setFixedWidth(450);
    filesGroup->addLayout(filesButtons);
    filesGroup->addWidget(recentFilesList);

    auto* const sessionsGroup = new QHBoxLayout();
    auto* const sessionsButtons = new QVBoxLayout();
    newSessionButton = new QPushButton(QStringLiteral("جلسة جديدة"), centralWidget);
    newSessionButton->setObjectName(QStringLiteral("NewSessionButton"));
    manageSessionsButton = new QPushButton(QStringLiteral("إدارة الجلسات"), centralWidget);
    manageSessionsButton->setObjectName(QStringLiteral("ManageSessionsButton"));
    sessionsButtons->addWidget(newSessionButton);
    sessionsButtons->addWidget(manageSessionsButton);
    sessionsButtons->addStretch();

    auto* const sessionsContent = new QWidget(centralWidget);
    sessionsContent->setFixedWidth(450);
    sessionsContentLayout = new QStackedLayout(sessionsContent);
    sessionsContentLayout->setContentsMargins(0, 0, 0, 0);
    savedSessionsList = new QListWidget(sessionsContent);
    savedSessionsList->setObjectName(QStringLiteral("SavedSessionsList"));
    savedSessionsList->setWordWrap(true);
    noSessionsLabel = new QLabel(QStringLiteral("لا توجد جلسات محفوظة"), sessionsContent);
    noSessionsLabel->setObjectName(QStringLiteral("sessionsBoxLabel"));
    noSessionsLabel->setAlignment(Qt::AlignCenter);
    noSessionsLabel->setMinimumHeight(100);
    sessionsContentLayout->addWidget(savedSessionsList);
    sessionsContentLayout->addWidget(noSessionsLabel);

    sessionsGroup->addLayout(sessionsButtons);
    sessionsGroup->addWidget(sessionsContent);

    mainContentLayout->addLayout(filesGroup);
    mainContentLayout->addLayout(sessionsGroup);

    showOnStartupCheck = new QCheckBox(QStringLiteral("إظهار صفحة الترحيب عند بدء البرنامج"), centralWidget);
    showOnStartupCheck->setChecked(true);
    showOnStartupCheck->setDisabled(true);

    mainLayout->addStretch(1);
    auto* const headerCentering = new QHBoxLayout();
    headerCentering->addStretch();
    headerCentering->addLayout(headerContent);
    headerCentering->addStretch();
    mainLayout->addLayout(headerCentering);
    mainLayout->addSpacing(30);
    auto* const contentCentering = new QHBoxLayout();
    contentCentering->addStretch();
    contentCentering->addLayout(mainContentLayout);
    contentCentering->addStretch();
    mainLayout->addLayout(contentCentering);
    mainLayout->addStretch(1);

    setWindowTitle(QStringLiteral("صفحة الترحيب — محرر طيف"));
    if (QScreen* const screen = QGuiApplication::primaryScreen()) {
        const QRect geometry = screen->availableGeometry();
        setGeometry(geometry.adjusted(300, 80, -300, -80));
    }

    connect(newFileButton, &QPushButton::clicked, this, &WelcomeWindow::handleNewFileRequest);
    connect(openFileButton, &QPushButton::clicked, this, &WelcomeWindow::handleOpenFileRequest);
    connect(openFolderButton, &QPushButton::clicked, this, &WelcomeWindow::handleOpenFolderRequest);
    connect(recentFilesList, &QListWidget::itemDoubleClicked,
            this, &WelcomeWindow::onRecentFileClicked);
    connect(newSessionButton, &QPushButton::clicked, this, &WelcomeWindow::createSession);
    connect(manageSessionsButton, &QPushButton::clicked, this, &WelcomeWindow::manageSessions);
    connect(savedSessionsList, &QListWidget::itemDoubleClicked,
            this, &WelcomeWindow::openSelectedSession);

    refreshSessions();
}

void WelcomeWindow::setupStyle()
{
    setStyleSheet(QStringLiteral(R"(
        QWidget {
            background-color: #0f172a;
            color: #f1f5f9;
            font-family: "Tajawal", "Noto Kufi Arabic", Roboto, Helvetica, Arial, sans-serif;
        }
        QLabel { color: #94a3b8; font-size: 13px; }
        QLabel#titleLabel { color: #ffffff; font-size: 24px; font-weight: bold; margin-bottom: 10px; }
        QPushButton {
            min-width: 90px;
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            padding: 8px 16px;
            border-radius: 6px;
            font-weight: 500;
        }
        QPushButton:hover { background-color: #334155; border-color: #3b82f6; color: #ffffff; }
        QPushButton:pressed { background-color: #0f172a; color: #3b82f6; }
        QPushButton#primaryButton { background-color: #3b82f6; color: #ffffff; border: none; }
        QPushButton#primaryButton:hover { background-color: #2563eb; }
        QListWidget, QLabel#sessionsBoxLabel {
            background-color: #1e293b;
            border: 1px solid #334155;
            border-radius: 12px;
            color: #f1f5f9;
            outline: none;
            padding: 5px;
            font-size: 14px;
        }
        QListWidget::item { background-color: transparent; color: #94a3b8; padding: 8px; border-radius: 8px; margin: 2px 0; }
        QListWidget::item:hover { background-color: #334155; color: #ffffff; }
        QListWidget::item:selected { background-color: #3b82f6; color: #ffffff; border: 1px solid #60a5fa; }
    )"));
}

void WelcomeWindow::refreshSessions()
{
    const QVector<SavedSession> sessions = sessionStore.loadAll();
    savedSessionsList->clear();
    for (const SavedSession& session : sessions) {
        addSessionItem(savedSessionsList, session);
    }
    sessionsContentLayout->setCurrentWidget(sessions.isEmpty()
        ? static_cast<QWidget*>(noSessionsLabel)
        : static_cast<QWidget*>(savedSessionsList));
}

bool WelcomeWindow::editSession(SavedSession session, const bool isNew)
{
    SessionEditorDialog dialog(this);
    dialog.setSession(session);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    QString errorMessage;
    const bool saved = isNew
        ? sessionStore.create(dialog.session(), &errorMessage)
        : sessionStore.update(dialog.session(), &errorMessage);
    if (!saved) {
        QMessageBox::warning(this, QStringLiteral("تعذر حفظ الجلسة"), errorMessage);
        return false;
    }
    refreshSessions();
    return true;
}

void WelcomeWindow::createSession()
{
    editSession({}, true);
}

void WelcomeWindow::manageSessions()
{
    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("SessionManagementDialog"));
    dialog.setWindowTitle(QStringLiteral("إدارة الجلسات"));
    dialog.setModal(true);
    dialog.setLayoutDirection(Qt::RightToLeft);
    dialog.resize(540, 420);

    auto* const layout = new QVBoxLayout(&dialog);
    auto* const list = new QListWidget(&dialog);
    list->setObjectName(QStringLiteral("ManagedSessionsList"));
    auto* const controls = new QHBoxLayout();
    auto* const openButton = new QPushButton(QStringLiteral("فتح"), &dialog);
    auto* const editButton = new QPushButton(QStringLiteral("تعديل"), &dialog);
    auto* const deleteButton = new QPushButton(QStringLiteral("حذف"), &dialog);
    auto* const createButton = new QPushButton(QStringLiteral("جلسة جديدة"), &dialog);
    controls->addWidget(openButton);
    controls->addWidget(editButton);
    controls->addWidget(deleteButton);
    controls->addStretch();
    controls->addWidget(createButton);
    auto* const buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("إغلاق"));

    layout->addWidget(list, 1);
    layout->addLayout(controls);
    layout->addWidget(buttons);

    const auto populate = [this, list]() {
        list->clear();
        for (const SavedSession& session : sessionStore.loadAll()) {
            addSessionItem(list, session);
        }
    };
    const auto selectedSession = [this, list]() -> std::optional<SavedSession> {
        const QVector<SavedSession> sessions = sessionStore.loadAll();
        if (const SavedSession* const selected = sessionForItem(sessions, list->currentItem())) {
            return *selected;
        }
        return std::nullopt;
    };

    connect(createButton, &QPushButton::clicked, &dialog, [this, populate]() {
        if (editSession({}, true)) {
            populate();
        }
    });
    connect(editButton, &QPushButton::clicked, &dialog, [this, selectedSession, populate]() {
        if (const auto selected = selectedSession(); selected.has_value() && editSession(*selected, false)) {
            populate();
        }
    });
    connect(deleteButton, &QPushButton::clicked, &dialog, [this, selectedSession, populate]() {
        const auto selected = selectedSession();
        if (!selected.has_value()) {
            return;
        }
        const auto answer = QMessageBox::question(
            this, QStringLiteral("حذف جلسة"),
            QStringLiteral("هل تريد حذف الجلسة «%1»؟\nلن يتم حذف أي ملفات.")
                .arg(selected->displayName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
        QString errorMessage;
        if (!sessionStore.remove(selected->id, &errorMessage)) {
            QMessageBox::warning(this, QStringLiteral("تعذر حذف الجلسة"), errorMessage);
            return;
        }
        refreshSessions();
        populate();
    });
    std::optional<SavedSession> sessionToOpen;
    connect(openButton, &QPushButton::clicked, &dialog,
            [&dialog, &sessionToOpen, selectedSession]() {
                if (const auto selected = selectedSession(); selected.has_value()) {
                    sessionToOpen = *selected;
                    dialog.accept();
                }
            });
    connect(list, &QListWidget::itemDoubleClicked, &dialog,
            [&dialog, &sessionToOpen, selectedSession](QListWidgetItem*) {
                if (const auto selected = selectedSession(); selected.has_value()) {
                    sessionToOpen = *selected;
                    dialog.accept();
                }
            });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    populate();
    dialog.exec();
    refreshSessions();
    if (sessionToOpen.has_value()) {
        openSession(*sessionToOpen);
    }
}

void WelcomeWindow::openSelectedSession(QListWidgetItem* const item)
{
    const QVector<SavedSession> sessions = sessionStore.loadAll();
    if (const SavedSession* const selected = sessionForItem(sessions, item)) {
        openSession(*selected);
    }
}

void WelcomeWindow::openSession(const SavedSession& session)
{
    auto* const editor = new Taif({}, nullptr, false);
    const SessionRestoreResult restoreResult = editor->restoreSession(session);
    editor->show();

    if (!restoreResult.unavailableFilePaths.isEmpty()) {
        QMessageBox::warning(editor, QStringLiteral("ملفات غير متاحة"),
                             QStringLiteral("تعذر فتح الملفات التالية:\n%1")
                                 .arg(restoreResult.unavailableFilePaths.join(u'\n')));
    }
    close();
}

void WelcomeWindow::onRecentFileClicked(QListWidgetItem* const item)
{
    const QString filePath = item->text();
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(this, QStringLiteral("ملف غير موجود"),
                             QStringLiteral("تعذر العثور على الملف:\n%1\n\nربما تم نقله أو حذفه.")
                                 .arg(filePath));
        return;
    }

    auto* const editor = new Taif(filePath);
    editor->show();
    close();
}

void WelcomeWindow::handleNewFileRequest()
{
    auto* const editor = new Taif();
    editor->show();
    close();
}

void WelcomeWindow::handleOpenFileRequest()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("فتح ملف"), {},
        QStringLiteral("ملف ألف (*.alif *.aliflib);;كل الملفات (*)"));
    if (filePath.isEmpty()) {
        return;
    }

    auto* const editor = new Taif(filePath);
    editor->show();
    close();
}

void WelcomeWindow::handleOpenFolderRequest()
{
    const QString folderPath = QFileDialog::getExistingDirectory(this, QStringLiteral("فتح مجلد"));
    if (folderPath.isEmpty()) {
        return;
    }

    auto* const editor = new Taif();
    editor->loadFolder(folderPath);
    editor->show();
    close();
}

void WelcomeWindow::closeEvent(QCloseEvent* const event)
{
    event->accept();
}

WelcomeWindow::~WelcomeWindow() = default;
