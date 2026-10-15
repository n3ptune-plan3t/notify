#pragma once

#include <QWidget>
#include <QVariantMap>

class NotificationServer;
class AppConfig;
class QLabel;

class NotificationPopup final : public QWidget
{
    Q_OBJECT
public:
    explicit NotificationPopup(NotificationServer *server, AppConfig *config,
                               const QVariantMap &notification, QWidget *parent = nullptr);

    [[nodiscard]] uint notificationId() const noexcept { return m_id; }
    void setStackOffset(int offset);

private:
    void applyTheme();
    void placeWindow(int offset = 0);
    NotificationServer *m_server;
    AppConfig *m_config;
    QVariantMap m_notification;
    uint m_id = 0;
};
