#include "notificationpopup.h"
#include "notificationserver.h"
#include "appconfig.h"

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QColor>
#include <QVariantList>
#include <QCloseEvent>

namespace {
QString valueText(const QVariantMap &map, const char *key)
{
    return map.value(QString::fromLatin1(key)).toString();
}

QString colorValue(const AppConfig *config, const QString &key, const QString &fallback)
{
    const QColor color(config->value(key, fallback).toString());
    return color.isValid() ? color.name(QColor::HexRgb) : fallback;
}
}

NotificationPopup::NotificationPopup(NotificationServer *server, AppConfig *config,
                                     const QVariantMap &notification, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint),
      m_server(server), m_config(config), m_notification(notification),
      m_id(notification.value(QStringLiteral("id")).toUInt())
{
    setObjectName(QStringLiteral("notificationPopup"));
    setWindowTitle(valueText(notification, "summary").isEmpty()
                   ? QStringLiteral("Notification") : valueText(notification, "summary"));
    setAttribute(Qt::WA_DeleteOnClose);
    setAutoFillBackground(true);
    setFixedWidth(qMax(260, config->value(QStringLiteral("popup/width"), 360).toInt()));

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(14, 12, 14, 12);
    outer->setSpacing(8);

    auto *header = new QHBoxLayout;
    const QString appName = valueText(notification, "appName").isEmpty()
                            ? QStringLiteral("Notification") : valueText(notification, "appName");
    auto *appLabel = new QLabel(appName, this);
    appLabel->setObjectName(QStringLiteral("popupApp"));
    appLabel->setWordWrap(true);
    header->addWidget(appLabel, 1);
    auto *closeButton = new QPushButton(QStringLiteral("×"), this);
    closeButton->setFixedSize(28, 26);
    closeButton->setToolTip(QStringLiteral("Close popup; keep notification in history"));
    header->addWidget(closeButton);
    outer->addLayout(header);

    auto *summary = new QLabel(valueText(notification, "summary"), this);
    summary->setObjectName(QStringLiteral("popupSummary"));
    summary->setWordWrap(true);
    outer->addWidget(summary);

    const QString bodyText = valueText(notification, "body");
    if (!bodyText.isEmpty()) {
        auto *body = new QLabel(bodyText, this);
        body->setWordWrap(true);
        body->setTextInteractionFlags(Qt::TextSelectableByMouse);
        outer->addWidget(body);
    }

    const QVariantList actions = notification.value(QStringLiteral("actions")).toList();
    if (!actions.isEmpty()) {
        auto *actionsRow = new QHBoxLayout;
        for (const QVariant &value : actions) {
            const QVariantMap action = value.toMap();
            const QString key = action.value(QStringLiteral("key")).toString();
            auto *button = new QPushButton(action.value(QStringLiteral("label")).toString(), this);
            connect(button, &QPushButton::clicked, this, [this, key] {
                m_server->invokeAction(m_id, key);
                close();
            });
            actionsRow->addWidget(button);
        }
        outer->addLayout(actionsRow);
    }

    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
    applyTheme();
    adjustSize();
    placeWindow();
}

void NotificationPopup::applyTheme()
{
    const QString bg = colorValue(m_config, QStringLiteral("popup/background"),
                                  QStringLiteral("#20232b"));
    const QString card = colorValue(m_config, QStringLiteral("ui/card"),
                                    QStringLiteral("#242832"));
    const QString fg = colorValue(m_config, QStringLiteral("popup/foreground"),
                                  QStringLiteral("#f1f2f6"));
    const QString secondary = colorValue(m_config, QStringLiteral("ui/secondary"),
                                         QStringLiteral("#aab0bf"));
    const QString border = colorValue(m_config, QStringLiteral("popup/border"),
                                      QStringLiteral("#454b59"));
    const QString urgentBg = colorValue(m_config, QStringLiteral("ui/urgentBackground"),
                                        QStringLiteral("#3a252b"));
    const QString urgentBorder = colorValue(m_config, QStringLiteral("ui/urgentBorder"),
                                            QStringLiteral("#a84e60"));
    const bool urgent = m_notification.value(QStringLiteral("urgency")).toInt() == 2;
    setStyleSheet(QStringLiteral(
        "QWidget#notificationPopup{background:%1;color:%2;border:1px solid %3;}"
        "QLabel{color:%2;border:0;background:transparent;}"
        "QLabel#popupApp{color:%4;font-size:11px;}"
        "QLabel#popupSummary{font-weight:600;font-size:14px;}"
        "QPushButton{color:%2;background:%5;border:1px solid %3;border-radius:5px;padding:5px 8px;}"
        "QPushButton:hover{border-color:%4;}"
    ).arg(urgent ? urgentBg : bg, fg, urgent ? urgentBorder : border, secondary, card));
    // Keep the popup fully opaque; no translucent-background attributes are set.
    QPalette p = palette();
    p.setColor(QPalette::Window, QColor(urgent ? urgentBg : bg));
    setPalette(p);
}

void NotificationPopup::setStackOffset(int offset)
{
    placeWindow(offset);
}

void NotificationPopup::placeWindow(int offset)
{
    // X11 generally honours this geometry. Wayland compositors may choose the
    // placement of ordinary top-level windows; no unsupported positioning is assumed.
    const QString position = m_config->value(QStringLiteral("popup/position"),
                                              QStringLiteral("top-right")).toString();
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    const QRect area = screen->availableGeometry();
    const int margin = qMax(0, m_config->value(QStringLiteral("popup/margin"), 12).toInt());
    int x = area.right() - width() - margin + 1;
    int y = area.top() + margin + offset;
    if (position.contains(QStringLiteral("left"), Qt::CaseInsensitive))
        x = area.left() + margin;
    if (position.contains(QStringLiteral("bottom"), Qt::CaseInsensitive))
        y = area.bottom() - height() - margin - offset + 1;
    move(x, y);
}
