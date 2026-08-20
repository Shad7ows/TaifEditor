#include "SessionStore.h"

#include <algorithm>
#include <utility>

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QSet>
#include <QUuid>
#include <QVariantMap>

namespace {

constexpr int kSchemaVersion = 1;
constexpr auto kSessionsKey = "SavedSessions";
constexpr auto kSchemaVersionKey = "schemaVersion";
constexpr auto kEntriesKey = "entries";

QString normalizedName(const QString& name)
{
    return name.trimmed();
}

QString nameKey(const QString& name)
{
    return normalizedName(name).toCaseFolded();
}

void assignError(QString* const errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

} // namespace

SessionStore::SessionStore(SettingsScope settingsScope)
    : scope(std::move(settingsScope))
{
}

QVector<SavedSession> SessionStore::loadAll() const
{
    QSettings settings = makeSettings();
    settings.beginGroup(settingsGroup());
    const QVariantList entries = settings.value(QString::fromLatin1(kEntriesKey)).toList();
    settings.endGroup();

    QVector<SavedSession> sessions;
    QSet<QString> knownIds;
    QSet<QString> knownNames;
    sessions.reserve(entries.size());

    for (const QVariant& entry : entries) {
        const QVariantMap map = entry.toMap();
        SavedSession session;
        session.id = map.value(QStringLiteral("id")).toString().trimmed();
        session.displayName = normalizedName(map.value(QStringLiteral("displayName")).toString());
        session.filePaths = map.value(QStringLiteral("filePaths")).toStringList();
        session.activeFilePath = map.value(QStringLiteral("activeFilePath")).toString();
        session.updatedAt = QDateTime::fromString(
            map.value(QStringLiteral("updatedAt")).toString(), Qt::ISODateWithMs);
        session = normalize(std::move(session));

        const QString sessionNameKey = nameKey(session.displayName);
        if (session.id.isEmpty() || session.displayName.isEmpty()
            || knownIds.contains(session.id) || knownNames.contains(sessionNameKey)) {
            continue;
        }

        knownIds.insert(session.id);
        knownNames.insert(sessionNameKey);
        sessions.append(std::move(session));
    }

    return sessions;
}

bool SessionStore::saveAll(const QVector<SavedSession>& sessions, QString* const errorMessage) const
{
    QVector<SavedSession> normalizedSessions;
    normalizedSessions.reserve(sessions.size());
    QSet<QString> ids;
    QSet<QString> names;

    for (SavedSession session : sessions) {
        session = normalize(std::move(session));
        const QString sessionNameKey = nameKey(session.displayName);
        if (session.id.isEmpty() || session.displayName.isEmpty()) {
            assignError(errorMessage, QStringLiteral("يجب أن تحتوي كل جلسة على اسم ومعرف صالحين."));
            return false;
        }
        if (ids.contains(session.id) || names.contains(sessionNameKey)) {
            assignError(errorMessage, QStringLiteral("توجد جلسة أخرى بالاسم نفسه."));
            return false;
        }
        ids.insert(session.id);
        names.insert(sessionNameKey);
        normalizedSessions.append(std::move(session));
    }

    QVariantList entries;
    entries.reserve(normalizedSessions.size());
    for (const SavedSession& session : normalizedSessions) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), session.id);
        map.insert(QStringLiteral("displayName"), session.displayName);
        map.insert(QStringLiteral("filePaths"), session.filePaths);
        map.insert(QStringLiteral("activeFilePath"), session.activeFilePath);
        map.insert(QStringLiteral("updatedAt"), session.updatedAt.toString(Qt::ISODateWithMs));
        entries.append(map);
    }

    QSettings settings = makeSettings();
    settings.remove(settingsGroup());
    settings.beginGroup(settingsGroup());
    settings.setValue(QString::fromLatin1(kSchemaVersionKey), kSchemaVersion);
    settings.setValue(QString::fromLatin1(kEntriesKey), entries);
    settings.endGroup();
    settings.sync();

    if (settings.status() != QSettings::NoError) {
        assignError(errorMessage, QStringLiteral("تعذر حفظ بيانات الجلسات."));
        return false;
    }
    return true;
}

bool SessionStore::create(SavedSession session, QString* const errorMessage) const
{
    QVector<SavedSession> sessions = loadAll();
    session = normalize(std::move(session));
    if (session.displayName.isEmpty()) {
        assignError(errorMessage, QStringLiteral("اسم الجلسة مطلوب."));
        return false;
    }
    if (!isDisplayNameAvailable(sessions, session.displayName)) {
        assignError(errorMessage, QStringLiteral("توجد جلسة أخرى بهذا الاسم."));
        return false;
    }

    session.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    session.updatedAt = QDateTime::currentDateTimeUtc();
    sessions.append(std::move(session));
    return saveAll(sessions, errorMessage);
}

bool SessionStore::update(SavedSession session, QString* const errorMessage) const
{
    QVector<SavedSession> sessions = loadAll();
    session = normalize(std::move(session));
    if (session.id.isEmpty() || session.displayName.isEmpty()) {
        assignError(errorMessage, QStringLiteral("الجلسة غير صالحة."));
        return false;
    }
    if (!isDisplayNameAvailable(sessions, session.displayName, session.id)) {
        assignError(errorMessage, QStringLiteral("توجد جلسة أخرى بهذا الاسم."));
        return false;
    }

    for (SavedSession& existing : sessions) {
        if (existing.id == session.id) {
            session.updatedAt = QDateTime::currentDateTimeUtc();
            existing = std::move(session);
            return saveAll(sessions, errorMessage);
        }
    }

    assignError(errorMessage, QStringLiteral("تعذر العثور على الجلسة المطلوبة."));
    return false;
}

bool SessionStore::remove(const QString& id, QString* const errorMessage) const
{
    QVector<SavedSession> sessions = loadAll();
    for (qsizetype index = 0; index < sessions.size(); ++index) {
        if (sessions.at(index).id == id) {
            sessions.removeAt(index);
            return saveAll(sessions, errorMessage);
        }
    }

    assignError(errorMessage, QStringLiteral("تعذر العثور على الجلسة المطلوبة."));
    return false;
}

QString SessionStore::normalizePath(const QString& path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const QFileInfo fileInfo(trimmed);
    return QDir::cleanPath(fileInfo.absoluteFilePath());
}

SavedSession SessionStore::normalize(SavedSession session)
{
    session.id = session.id.trimmed();
    session.displayName = normalizedName(session.displayName);

    QStringList paths;
    QSet<QString> pathKeys;
    paths.reserve(session.filePaths.size());
    for (const QString& path : std::as_const(session.filePaths)) {
        const QString normalizedPath = normalizePath(path);
        const QString pathKey = normalizedPath.toCaseFolded();
        if (!normalizedPath.isEmpty() && !pathKeys.contains(pathKey)) {
            pathKeys.insert(pathKey);
            paths.append(normalizedPath);
        }
    }
    session.filePaths = std::move(paths);

    session.activeFilePath = normalizePath(session.activeFilePath);
    if (!session.activeFilePath.isEmpty()
        && !pathKeys.contains(session.activeFilePath.toCaseFolded())) {
        session.activeFilePath.clear();
    }
    if (!session.updatedAt.isValid()) {
        session.updatedAt = QDateTime::currentDateTimeUtc();
    }
    return session;
}

bool SessionStore::isDisplayNameAvailable(const QVector<SavedSession>& sessions,
                                          const QString& displayName,
                                          const QString& excludedId)
{
    const QString requestedKey = nameKey(displayName);
    return !requestedKey.isEmpty()
        && std::none_of(sessions.cbegin(), sessions.cend(),
                        [&requestedKey, &excludedId](const SavedSession& session) {
                            return session.id != excludedId
                                && nameKey(session.displayName) == requestedKey;
                        });
}

QString SessionStore::settingsGroup() const
{
    return QString::fromLatin1(kSessionsKey);
}

QSettings SessionStore::makeSettings() const
{
    if (!scope.fileName.isEmpty()) {
        return QSettings(scope.fileName, QSettings::IniFormat);
    }
    return QSettings(scope.organization, scope.application);
}
