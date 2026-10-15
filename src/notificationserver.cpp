#include "notificationserver.h"

#include <QDateTime>
#include <QBuffer>
#include <QDBusArgument>
#include <QImage>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <utility>
#include <QUrl>

namespace {
QString historyPath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);
    return base + QStringLiteral("/notifications.json");
}

QString saveImageHint(const QVariantMap &hints, uint id)
{
    QVariant raw = hints.value(QStringLiteral("image-data"));
    if (!raw.isValid()) raw = hints.value(QStringLiteral("image_data"));
    if (!raw.isValid()) return {};

    int width = 0, height = 0, rowStride = 0, bitsPerSample = 0, channels = 0;
    bool hasAlpha = false;
    QByteArray pixels;
    const QDBusArgument argument = raw.value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::StructureType) {
        QDBusArgument copy = argument;
        copy.beginStructure();
        copy >> width >> height >> rowStride >> hasAlpha >> bitsPerSample >> channels >> pixels;
        copy.endStructure();
    } else {
        return {};
    }

    constexpr int maxDimension = 2048;
    if (width <= 0 || height <= 0 || width > maxDimension || height > maxDimension ||
        bitsPerSample != 8 || (channels != 3 && channels != 4) ||
        rowStride < width * channels ||
        static_cast<qint64>(rowStride) * height > pixels.size()) return {};

    const QImage::Format format = channels == 4 ? QImage::Format_RGBA8888 : QImage::Format_RGB888;
    QImage image(reinterpret_cast<const uchar *>(pixels.constData()), width, height,
                 rowStride, format);
    if (image.isNull()) return {};
    const QString iconDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + QStringLiteral("/icons");
    if (!QDir().mkpath(iconDir)) return {};
    const QString path = iconDir + QStringLiteral("/notification-%1.png").arg(id);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return {};
    QBuffer buffer;
    QByteArray png;
    buffer.setBuffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG") ||
        file.write(png) != png.size() || !file.commit()) return {};
    return QUrl::fromLocalFile(path).toString();
}

QVariantMap sanitizeMap(QVariantMap item)
{
    item.insert(QStringLiteral("appName"), item.value(QStringLiteral("appName")).toString().left(256));
    item.insert(QStringLiteral("appIcon"), item.value(QStringLiteral("appIcon")).toString().left(2048));
    item.insert(QStringLiteral("summary"), item.value(QStringLiteral("summary")).toString().left(2048));
    item.insert(QStringLiteral("body"), item.value(QStringLiteral("body")).toString().left(8192));
    return item;
}
}

NotificationServer::NotificationServer(QObject *parent)
    : QObject(parent), m_settings(QStringLiteral("QtNotifyCenter"), QStringLiteral("QtNotifyCenter"))
{
    loadHistory();
}

QVariantList NotificationServer::notifications() const { return m_notifications; }
bool NotificationServer::doNotDisturb() const noexcept
{
    return m_settings.value(QStringLiteral("doNotDisturb"), false).toBool();
}

uint NotificationServer::Notify(const QString &appName, uint replacesId, const QString &appIcon,
                                const QString &summary, const QString &body,
                                const QStringList &actions, const QVariantMap &hints,
                                int expireTimeout)
{
    Q_UNUSED(expireTimeout);
    uint id = replacesId;
    qsizetype existingIndex = -1;
    if (replacesId != 0) {
        for (qsizetype i = 0; i < m_notifications.size(); ++i) {
            if (m_notifications.at(i).toMap().value(QStringLiteral("id")).toUInt() == replacesId) {
                existingIndex = i;
                break;
            }
        }
    }
    if (existingIndex < 0) {
        id = m_nextId++;
        if (m_nextId == 0) m_nextId = 1;
    }

    QVariantList actionList;
    for (qsizetype i = 0; i + 1 < actions.size(); i += 2) {
        actionList.append(QVariantMap{{QStringLiteral("key"), actions.at(i).left(256)},
                                      {QStringLiteral("label"), actions.at(i + 1).left(512)}});
    }

    int urgency = 1;
    const QVariant urgencyHint = hints.value(QStringLiteral("urgency"));
    if (urgencyHint.isValid()) urgency = qBound(0, urgencyHint.toInt(), 2);

    QString icon = appIcon;
    if (icon.isEmpty()) {
        icon = hints.value(QStringLiteral("image-path")).toString();
        if (icon.isEmpty()) icon = hints.value(QStringLiteral("image_path")).toString();
    }
    if (icon.isEmpty()) icon = saveImageHint(hints, id);

    const QDateTime now = QDateTime::currentDateTime();
    QVariantMap item{
        {QStringLiteral("id"), id},
        {QStringLiteral("appName"), appName.left(256)},
        {QStringLiteral("appIcon"), icon.left(2048)},
        {QStringLiteral("summary"), summary.left(2048)},
        {QStringLiteral("body"), body.left(8192)},
        {QStringLiteral("actions"), actionList},
        {QStringLiteral("urgency"), urgency},
        {QStringLiteral("timestamp"), now.toString(QStringLiteral("hh:mm"))},
        {QStringLiteral("timestampMs"), now.toMSecsSinceEpoch()}
    };
    item = sanitizeMap(std::move(item));

    if (existingIndex >= 0) m_notifications[existingIndex] = item;
    else m_notifications.prepend(item);

    while (m_notifications.size() > maxNotifications) {
        const uint removedId = m_notifications.last().toMap().value(QStringLiteral("id")).toUInt();
        m_notifications.removeLast();
        emit NotificationClosed(removedId, 2);
    }
    saveHistory();
    emit notificationsChanged();
    emit notificationReceived(item);
    return id;
}

void NotificationServer::CloseNotification(uint id)
{
    for (qsizetype i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications.at(i).toMap().value(QStringLiteral("id")).toUInt() == id) {
            m_notifications.removeAt(i);
            saveHistory();
            emit notificationsChanged();
            emit NotificationClosed(id, 2);
            return;
        }
    }
}

bool NotificationServer::invokeAction(uint id, const QString &actionKey)
{
    for (const QVariant &value : std::as_const(m_notifications)) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("id")).toUInt() != id) continue;
        const QVariantList actions = item.value(QStringLiteral("actions")).toList();
        if (actionKey == QStringLiteral("default")) {
            emit ActionInvoked(id, actionKey);
            return true;
        }
        for (const QVariant &actionValue : actions) {
            if (actionValue.toMap().value(QStringLiteral("key")).toString() == actionKey) {
                emit ActionInvoked(id, actionKey);
                return true;
            }
        }
        return false;
    }
    return false;
}

QString NotificationServer::resolveIcon(const QString &name) const
{
    if (name.isEmpty()) return {};
    const QUrl url(name);
    if (url.isLocalFile()) return QFileInfo(url.toLocalFile()).isFile() ? url.toString() : QString{};
    // Do not fetch remote icons: notifications must not trigger implicit network requests.
    if (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https")) return {};
    if (QFileInfo(name).isAbsolute())
        return QFileInfo(name).isFile() ? QUrl::fromLocalFile(name).toString() : QString{};
    // Let the theme provider resolve the name, including fallback icons.
    return QStringLiteral("image://theme/") + name;
}

QStringList NotificationServer::GetCapabilities() const
{
    return {QStringLiteral("actions"), QStringLiteral("body"), QStringLiteral("body-markup"),
            QStringLiteral("icon-static"), QStringLiteral("persistence")};
}

QString NotificationServer::GetServerInformation(QString &vendor, QString &version, QString &specVersion) const
{
    vendor = QStringLiteral("QtNotifyCenter");
    version = QStringLiteral("0.2.0");
    specVersion = QStringLiteral("1.2");
    return QStringLiteral("Qt Notify Center");
}

void NotificationServer::ClearNotifications()
{
    const QVariantList old = std::exchange(m_notifications, {});
    saveHistory();
    emit notificationsChanged();
    for (const QVariant &value : old)
        emit NotificationClosed(value.toMap().value(QStringLiteral("id")).toUInt(), 2);
}

void NotificationServer::setDoNotDisturb(bool enabled)
{
    if (doNotDisturb() == enabled) return;
    m_settings.setValue(QStringLiteral("doNotDisturb"), enabled);
    m_settings.sync();
    emit doNotDisturbChanged();
}

void NotificationServer::ToggleDoNotDisturb() { setDoNotDisturb(!doNotDisturb()); }

void NotificationServer::loadHistory()
{
    QFile file(historyPath());
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) return;
    const QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        if (!value.isObject()) continue;
        QVariantMap item = value.toObject().toVariantMap();
        if (!item.contains(QStringLiteral("id")) || !item.contains(QStringLiteral("summary"))) continue;
        m_notifications.append(sanitizeMap(std::move(item)));
        m_nextId = qMax(m_nextId, m_notifications.last().toMap().value(QStringLiteral("id")).toUInt() + 1);
    }
    while (m_notifications.size() > maxNotifications) m_notifications.removeLast();
}

void NotificationServer::saveHistory() const
{
    QJsonArray array;
    for (const QVariant &value : m_notifications) array.append(QJsonObject::fromVariantMap(value.toMap()));
    QSaveFile file(historyPath());
    if (!file.open(QIODevice::WriteOnly)) return;
    if (file.write(QJsonDocument(array).toJson(QJsonDocument::Compact)) < 0) {
        file.cancelWriting();
        return;
    }
    file.commit();
}
