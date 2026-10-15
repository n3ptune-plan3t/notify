#pragma once
#include <QMainWindow>
#include <QVariantMap>
class NotificationServer;
class AppConfig;
class QVBoxLayout;
class QLabel;
class QPushButton;
class QWidget;
class NotificationPanel final : public QMainWindow {
    Q_OBJECT
public:
    explicit NotificationPanel(NotificationServer *server, AppConfig *config, QWidget *parent = nullptr);
public slots:
    void togglePanel();
    void showPanel();
    void hidePanel();
    void rebuild();
private:
    QWidget *makeNotificationCard(const QVariantMap &item);
    void applyTheme();
    void clearLayout(QVBoxLayout *layout);
    NotificationServer *m_server;
    AppConfig *m_config;
    QWidget *m_listHost = nullptr;
    QVBoxLayout *m_listLayout = nullptr;
    QLabel *m_countLabel = nullptr;
    QPushButton *m_dndButton = nullptr;
};
