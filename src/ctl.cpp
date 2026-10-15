#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QTextStream>

namespace {
constexpr auto serviceName = "org.example.QtNotifyCenter";
constexpr auto controlPath = "/org/example/QtNotifyCenter";
constexpr auto controlInterface = "org.example.QtNotifyCenter";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qt-notify-centerctl"));

    const QStringList args = app.arguments();
    const bool status = args.contains(QStringLiteral("--status"));
    const bool toggle = args.contains(QStringLiteral("--toggle"));
    const bool show = args.contains(QStringLiteral("--show"));
    const bool hide = args.contains(QStringLiteral("--hide"));
    const bool clear = args.contains(QStringLiteral("--clear"));
    const bool dnd = args.contains(QStringLiteral("--dnd"));
    if (!(status || toggle || show || hide || clear || dnd)) {
        QTextStream(stderr) << "Usage: qt-notify-centerctl --status|--toggle|--show|--hide|--clear|--dnd\n";
        return 2;
    }

    QDBusInterface control(serviceName, controlPath, controlInterface,
                           QDBusConnection::sessionBus());
    if (!control.isValid()) {
        QTextStream(stderr) << "Qt Notify Center is not running.\n";
        return 1;
    }

    const QString method = toggle ? QStringLiteral("TogglePanel")
                          : show ? QStringLiteral("ShowPanel")
                          : hide ? QStringLiteral("HidePanel")
                          : clear ? QStringLiteral("ClearNotifications")
                          : dnd ? QStringLiteral("ToggleDoNotDisturb")
                          : QStringLiteral("GetNotificationCount");
    const QDBusMessage reply = control.call(method);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        QTextStream(stderr) << reply.errorMessage() << '\n';
        return 1;
    }

    if (status) {
        if (reply.arguments().isEmpty()) return 1;
        const int count = reply.arguments().constFirst().toInt();
        const QDBusMessage dndReply = control.call(QStringLiteral("GetDoNotDisturb"));
        const bool dndEnabled = dndReply.type() != QDBusMessage::ErrorMessage
                                && !dndReply.arguments().isEmpty()
                                && dndReply.arguments().constFirst().toBool();
        QTextStream(stdout) << "{\"text\":\"" << (dndEnabled ? "󰂛 " : "󰂚 ") << count
                            << "\",\"tooltip\":\"" << (dndEnabled ? "Do Not Disturb enabled" : "Notifications")
                            << "\",\"class\":\"" << (dndEnabled ? "dnd" : "notifications") << "\"}\n";
    }
    return 0;
}
