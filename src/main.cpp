#include "notificationserver.h"
#include "panelcontroller.h"
#include "appconfig.h"
#include "notificationpanel.h"
#include "notificationpopup.h"
#include <QPointer>
#include <QList>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <signal.h>
#include <unistd.h>

namespace {
constexpr auto serviceName = "org.example.QtNotifyCenter";
constexpr auto notificationServiceName = "org.freedesktop.Notifications";
constexpr auto controlPath = "/org/example/QtNotifyCenter";

// Notify Waybar only on state changes. This is event-driven, not a polling loop.
void notifyWaybar()
{
    const QDir proc(QStringLiteral("/proc"));
    const QStringList entries = proc.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        bool ok = false;
        const qlonglong pid = entry.toLongLong(&ok);
        if (!ok || pid <= 1 || pid == QCoreApplication::applicationPid())
            continue;
        QFile comm(QStringLiteral("/proc/%1/comm").arg(pid));
        if (comm.open(QIODevice::ReadOnly) &&
            comm.readAll().trimmed() == QByteArrayLiteral("waybar")) {
            ::kill(static_cast<pid_t>(pid), SIGRTMIN + 8);
        }
    }
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qt-notify-center"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.3.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("QtNotifyCenter"));
    QApplication::setQuitOnLastWindowClosed(false);

    const auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCritical() << "A session D-Bus is required.";
        return 1;
    }
    const auto *busInterface = bus.interface();
    if (busInterface && (busInterface->isServiceRegistered(serviceName).value() ||
                         busInterface->isServiceRegistered(notificationServiceName).value())) {
        qCritical() << "A notification daemon or Qt Notify Center instance is already running.";
        return 1;
    }

    AppConfig config;
    NotificationServer server;
    NotificationPanel panel(&server, &config);
    PanelController controller(&server);
    QList<QPointer<NotificationPopup>> popups;
    QObject::connect(&server, &NotificationServer::notificationReceived, &app,
                     [&server, &config, &popups](const QVariantMap &notification) {
        if (!config.value(QStringLiteral("popup/enabled"), true).toBool() ||
            server.doNotDisturb())
            return;
        // Remove dead popup references without timers or polling.
        for (auto it = popups.begin(); it != popups.end();) {
            if (it->isNull()) it = popups.erase(it);
            else ++it;
        }
        auto *popup = new NotificationPopup(&server, &config, notification);
        int stackOffset = 0;
        const int gap = qMax(0, config.value(QStringLiteral("popup/gap"), 10).toInt());
        for (const auto &existing : popups) {
            if (!existing.isNull() && existing->isVisible())
                stackOffset += existing->height() + gap;
        }
        popup->setStackOffset(stackOffset);
        popups.append(QPointer<NotificationPopup>(popup));
        popup->show();
        popup->raise();
    });
    QObject::connect(&server, &NotificationServer::notificationsChanged, &app, [] { notifyWaybar(); });
    QObject::connect(&server, &NotificationServer::doNotDisturbChanged, &app, [] { notifyWaybar(); });
    QObject::connect(&controller, &PanelController::toggleRequested, &panel, &NotificationPanel::togglePanel);
    QObject::connect(&controller, &PanelController::showRequested, &panel, &NotificationPanel::showPanel);
    QObject::connect(&controller, &PanelController::hideRequested, &panel, &NotificationPanel::hidePanel);

    if (!bus.registerService(notificationServiceName) || !bus.registerService(serviceName)) {
        qCritical().noquote() << "Cannot register D-Bus service:" << bus.lastError().message();
        return 1;
    }
    if (!bus.registerObject(QStringLiteral("/org/freedesktop/Notifications"), &server,
                            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        qCritical().noquote() << "Cannot export notification interface:" << bus.lastError().message();
        return 1;
    }
    if (!bus.registerObject(QString::fromLatin1(controlPath), &controller,
                            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        qCritical().noquote() << "Cannot export control interface:" << bus.lastError().message();
        return 1;
    }
    return app.exec();
}
