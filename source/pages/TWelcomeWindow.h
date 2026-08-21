#pragma once

#include "SessionStore.h"

#include <QMainWindow>

class QCheckBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedLayout;

class WelcomeWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit WelcomeWindow(QWidget* parent = nullptr,
                           SessionStore::SettingsScope sessionScope = {});
    ~WelcomeWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void handleNewFileRequest();
    void handleOpenFileRequest();
    void handleOpenFolderRequest();
    void onRecentFileClicked(QListWidgetItem* item);
    void createSession();
    void manageSessions();
    void openSelectedSession(QListWidgetItem* item);

signals:
    void newDocumentRequested();
    void fileOpenRequested(QString filePath);
    void folderOpenRequested(QString folderPath);
    void sessionOpenRequested(SavedSession session);

private:
    void setupStyle();
    void refreshSessions();
    bool editSession(SavedSession session, bool isNew);
    void openSession(const SavedSession& session);

    QPushButton* newFileButton = nullptr;
    QPushButton* openFileButton = nullptr;
    QPushButton* openFolderButton = nullptr;
    QListWidget* recentFilesList = nullptr;

    QPushButton* newSessionButton = nullptr;
    QPushButton* manageSessionsButton = nullptr;
    QListWidget* savedSessionsList = nullptr;
    QLabel* noSessionsLabel = nullptr;
    QStackedLayout* sessionsContentLayout = nullptr;

    QCheckBox* showOnStartupCheck = nullptr;
    SessionStore sessionStore;
};
