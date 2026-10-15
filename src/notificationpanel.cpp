#include "notificationpanel.h"
#include "notificationserver.h"
#include "appconfig.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QColor>
#include <QVariantMap>
#include <QVariantList>
#include <QSizePolicy>

namespace {
QString textValue(const QVariantMap &m, const QString &k) { return m.value(k).toString(); }
QString colorValue(const AppConfig *c, const QString &key, const QString &fallback) {
    const QColor color(c->value(key, fallback).toString());
    return color.isValid() ? color.name(QColor::HexRgb) : fallback;
}
}

NotificationPanel::NotificationPanel(NotificationServer *server, AppConfig *config, QWidget *parent)
    : QMainWindow(parent), m_server(server), m_config(config)
{
    setWindowTitle(QStringLiteral("Notifications"));
    setObjectName(QStringLiteral("notificationPanel"));
    setMinimumSize(300, 360);
    resize(m_config->value(QStringLiteral("ui/panelWidth"), 410).toInt(),
           m_config->value(QStringLiteral("ui/panelHeight"), 640).toInt());

    auto *root = new QWidget(this);
    auto *outer = new QVBoxLayout(root);
    outer->setContentsMargins(14, 12, 14, 12);
    outer->setSpacing(10);

    auto *header = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("Notifications"), root);
    title->setObjectName(QStringLiteral("panelTitle"));
    m_countLabel = new QLabel(root);
    auto *clearButton = new QPushButton(QStringLiteral("Clear all"), root);
    m_dndButton = new QPushButton(root);
    auto *closeButton = new QPushButton(QStringLiteral("×"), root);
    closeButton->setToolTip(QStringLiteral("Hide notification center"));
    header->addWidget(title, 1);
    header->addWidget(m_countLabel);
    header->addWidget(m_dndButton);
    header->addWidget(clearButton);
    header->addWidget(closeButton);
    outer->addLayout(header);

    auto *line = new QFrame(root);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName(QStringLiteral("separator"));
    outer->addWidget(line);

    auto *scroll = new QScrollArea(root);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listHost = new QWidget(scroll);
    m_listLayout = new QVBoxLayout(m_listHost);
    m_listLayout->setContentsMargins(0, 0, 4, 0);
    m_listLayout->setSpacing(8);
    scroll->setWidget(m_listHost);
    outer->addWidget(scroll, 1);
    setCentralWidget(root);

    connect(clearButton, &QPushButton::clicked, m_server, &NotificationServer::ClearNotifications);
    connect(closeButton, &QPushButton::clicked, this, &NotificationPanel::hidePanel);
    connect(m_dndButton, &QPushButton::clicked, m_server, &NotificationServer::ToggleDoNotDisturb);
    connect(m_server, &NotificationServer::notificationsChanged, this, &NotificationPanel::rebuild);
    connect(m_server, &NotificationServer::doNotDisturbChanged, this, &NotificationPanel::rebuild);
    applyTheme();
    rebuild();
}

void NotificationPanel::applyTheme()
{
    const QString bg = colorValue(m_config, "ui/background", "#17191f");
    const QString card = colorValue(m_config, "ui/card", "#242832");
    const QString fg = colorValue(m_config, "ui/foreground", "#f1f2f6");
    const QString secondary = colorValue(m_config, "ui/secondary", "#aab0bf");
    const QString border = colorValue(m_config, "ui/border", "#343843");
    const QString urgent = colorValue(m_config, "ui/urgentBackground", "#3a252b");
    const QString urgentBorder = colorValue(m_config, "ui/urgentBorder", "#a84e60");
    setStyleSheet(QStringLiteral(
        "QMainWindow,QWidget#notificationPanel{background:%1;color:%2;}"
        "QLabel{color:%2;} QLabel#panelTitle{font-size:20px;font-weight:600;}"
        "QLabel#appHeading{color:%3;font-size:11px;font-weight:600;padding:5px 2px 0 2px;}"
        "QFrame#separator{color:%4;}"
        "QFrame#notificationCard{background:%5;border:1px solid %4;border-radius:10px;}"
        "QFrame#notificationCard[urgent=\"true\"]{background:%6;border:1px solid %7;}"
        "QPushButton{color:%2;background:%5;border:1px solid %4;border-radius:6px;padding:5px 8px;}"
        "QPushButton:hover{border-color:%3;} QLineEdit{color:%2;background:%1;border:1px solid %4;border-radius:6px;padding:6px;}"
    ).arg(bg, fg, secondary, border, card, urgent, urgentBorder));
}

void NotificationPanel::clearLayout(QVBoxLayout *layout)
{
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *w = item->widget()) delete w;
        delete item;
    }
}

QWidget *NotificationPanel::makeNotificationCard(const QVariantMap &item)
{
    auto *card = new QFrame(m_listHost);
    card->setObjectName(QStringLiteral("notificationCard"));
    card->setProperty("urgent", item.value(QStringLiteral("urgency")).toInt() == 2);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 9, 10, 10);
    layout->setSpacing(6);

    auto *top = new QHBoxLayout;
    auto *app = new QLabel(textValue(item, "appName").isEmpty() ? "Notification" : textValue(item, "appName"), card);
    app->setStyleSheet("color: palette(mid); font-size:11px;");
    app->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *time = new QLabel(textValue(item, "timestamp"), card);
    auto *dismiss = new QPushButton(QStringLiteral("×"), card);
    dismiss->setFixedWidth(28);
    dismiss->setToolTip(QStringLiteral("Dismiss notification"));
    const uint id = item.value(QStringLiteral("id")).toUInt();
    connect(dismiss, &QPushButton::clicked, m_server, [this, id] { m_server->CloseNotification(id); });
    top->addWidget(app, 1); top->addWidget(time); top->addWidget(dismiss);
    layout->addLayout(top);

    auto *summary = new QLabel(textValue(item, "summary"), card);
    summary->setWordWrap(true);
    summary->setStyleSheet("font-weight:600;");
    layout->addWidget(summary);

    const QString bodyText = textValue(item, "body");
    if (!bodyText.isEmpty()) {
        auto *body = new QLabel(bodyText, card);
        body->setWordWrap(true);
        body->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(body);
    }

    const QVariantList actions = item.value(QStringLiteral("actions")).toList();
    if (!actions.isEmpty()) {
        auto *row = new QHBoxLayout;
        for (const QVariant &v : actions) {
            const QVariantMap action = v.toMap();
            const QString key = textValue(action, "key");
            auto *button = new QPushButton(textValue(action, "label"), card);
            connect(button, &QPushButton::clicked, m_server, [this, id, key] {
                m_server->invokeAction(id, key);
            });
            row->addWidget(button);
        }
        row->addStretch(1);
        layout->addLayout(row);
    }
    return card;
}

void NotificationPanel::rebuild()
{
    clearLayout(m_listLayout);
    const QVariantList items = m_server->notifications();
    m_countLabel->setText(QString::number(items.size()));
    m_dndButton->setText(m_server->doNotDisturb() ? "DND: On" : "DND: Off");

    if (items.isEmpty()) {
        auto *empty = new QLabel(QStringLiteral("No notifications"), m_listHost);
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumHeight(160);
        m_listLayout->addWidget(empty);
        m_listLayout->addStretch(1);
        return;
    }

    // Group notifications by application, retaining newest-first order within each group.
    QMap<QString, QList<QVariantMap>> groups;
    QStringList appOrder;
    for (const QVariant &v : items) {
        const QVariantMap item = v.toMap();
        QString name = textValue(item, "appName");
        if (name.isEmpty()) name = QStringLiteral("Other");
        if (!groups.contains(name)) appOrder.append(name);
        groups[name].append(item);
    }
    for (const QString &name : appOrder) {
        auto *heading = new QLabel(QStringLiteral("%1 (%2)").arg(name).arg(groups.value(name).size()), m_listHost);
        heading->setObjectName(QStringLiteral("appHeading"));
        m_listLayout->addWidget(heading);
        for (const QVariantMap &item : groups.value(name))
            m_listLayout->addWidget(makeNotificationCard(item));
    }
    m_listLayout->addStretch(1);
}

void NotificationPanel::togglePanel() { isVisible() ? hidePanel() : showPanel(); }
void NotificationPanel::showPanel() { show(); raise(); activateWindow(); }
void NotificationPanel::hidePanel() { hide(); }
