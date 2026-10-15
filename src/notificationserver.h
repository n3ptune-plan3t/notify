#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QSettings>

class NotificationServer final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
    Q_PROPERTY(QVariantList notifications READ notifications NOTIFY notificationsChanged)
    Q_PROPERTY(bool doNotDisturb READ doNotDisturb WRITE setDoNotDisturb NOTIFY doNotDisturbChanged)

public:
    explicit NotificationServer(QObject *parent = nullptr);
    [[nodiscard]] QVariantList notifications() const;
    [[nodiscard]] bool doNotDisturb() const noexcept;

    Q_INVOKABLE bool invokeAction(uint id, const QString &actionKey);
    Q_INVOKABLE QString resolveIcon(const QString &name) const;

public slots:
    uint Notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expireTimeout);
    void CloseNotification(uint id);
    QStringList GetCapabilities() const;
    QString GetServerInformation(QString &vendor, QString &version, QString &specVersion) const;
    void ClearNotifications();
    void setDoNotDisturb(bool enabled);
    void ToggleDoNotDisturb();

signals:
    void notificationsChanged();
    void notificationReceived(const QVariantMap &notification);
    void doNotDisturbChanged();
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);

private:
    void loadHistory();
    void saveHistory() const;
    void appendHistory(const QVariantMap &item);
    QVariantList m_notifications;
    QSettings m_settings;
    uint m_nextId = 1;
    static constexpr qsizetype maxNotifications = 100;
};
