#pragma once
#include <QObject>
class NotificationServer;
class PanelController final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.example.QtNotifyCenter")
public:
    explicit PanelController(NotificationServer *server, QObject *parent = nullptr);
public slots:
    void TogglePanel();
    void ShowPanel();
    void HidePanel();
    void ClearNotifications();
    void ToggleDoNotDisturb();
    int GetNotificationCount() const;
    bool GetDoNotDisturb() const;
signals:
    void toggleRequested();
    void showRequested();
    void hideRequested();
private:
    NotificationServer *m_server;
};
