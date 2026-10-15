#include "appconfig.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QVariantMap>

namespace {
QString configFileName()
{
    const QString configDirectory = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                                    + QStringLiteral("/qt-notify-center");
    QDir().mkpath(configDirectory);
    return configDirectory + QStringLiteral("/config.ini");
}
}

AppConfig::AppConfig(QObject *parent)
    : QObject(parent),
      m_fileName(configFileName()),
      m_settings(m_fileName, QSettings::IniFormat)
{
    m_settings.setFallbacksEnabled(false);
    const QVariantMap defaults{
        {QStringLiteral("ui/background"), QStringLiteral("#17191f")},
        {QStringLiteral("ui/card"), QStringLiteral("#242832")},
        {QStringLiteral("ui/foreground"), QStringLiteral("#f1f2f6")},
        {QStringLiteral("ui/secondary"), QStringLiteral("#aab0bf")},
        {QStringLiteral("ui/border"), QStringLiteral("#343843")},
        {QStringLiteral("ui/urgentBackground"), QStringLiteral("#3a252b")},
        {QStringLiteral("ui/urgentBorder"), QStringLiteral("#a84e60")},
        {QStringLiteral("ui/panelWidth"), 410},
        {QStringLiteral("ui/panelHeight"), 640},
        {QStringLiteral("popup/enabled"), true},
        {QStringLiteral("popup/persistent"), true},
        {QStringLiteral("popup/width"), 360},
        {QStringLiteral("popup/margin"), 12},
        {QStringLiteral("popup/gap"), 10},
        {QStringLiteral("popup/position"), QStringLiteral("top-right")},
        {QStringLiteral("popup/background"), QStringLiteral("#20232b")},
        {QStringLiteral("popup/foreground"), QStringLiteral("#f1f2f6")},
        {QStringLiteral("popup/border"), QStringLiteral("#454b59")}
    };
    for (auto it = defaults.cbegin(); it != defaults.cend(); ++it) {
        if (!m_settings.contains(it.key()))
            m_settings.setValue(it.key(), it.value());
    }
    m_settings.sync();
}

QVariant AppConfig::value(const QString &key, const QVariant &fallback) const
{
    return m_settings.value(key, fallback);
}

QString AppConfig::fileName() const
{
    return m_fileName;
}
