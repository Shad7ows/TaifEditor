#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

class QSettings;

/** A durable, explicit set of files that can be restored into one editor window. */
struct SavedSession final {
    QString id;
    QString displayName;
    QStringList filePaths;
    QString activeFilePath;
    QDateTime updatedAt;
};

class SessionStore final {
public:
    struct SettingsScope final {
        QString organization = QStringLiteral("Alif");
        QString application = QStringLiteral("Taif");
        /** Optional INI path used by isolated tests; empty selects normal user settings. */
        QString fileName;
    };

    explicit SessionStore(SettingsScope scope = {});

    [[nodiscard]] QVector<SavedSession> loadAll() const;
    bool saveAll(const QVector<SavedSession>& sessions, QString* errorMessage = nullptr) const;

    bool create(SavedSession session, QString* errorMessage = nullptr) const;
    bool update(SavedSession session, QString* errorMessage = nullptr) const;
    bool remove(const QString& id, QString* errorMessage = nullptr) const;

    [[nodiscard]] static QString normalizePath(const QString& path);
    [[nodiscard]] static SavedSession normalize(SavedSession session);
    [[nodiscard]] static bool isDisplayNameAvailable(const QVector<SavedSession>& sessions,
                                                     const QString& displayName,
                                                     const QString& excludedId = {});

private:
    [[nodiscard]] QString settingsGroup() const;
    [[nodiscard]] QSettings makeSettings() const;

    SettingsScope scope;
};
