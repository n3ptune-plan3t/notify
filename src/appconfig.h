#pragma once

#include <QObject>
#include <QSettings>
#include <QVariant>

class AppConfig final : public QObject
{
    Q_OBJECT
public:
    explicit AppConfig(QObject *parent = nullptr);
    Q_INVOKABLE QVariant value(const QString &key, const QVariant &fallback = {}) const;
    [[nodiscard]] QString fileName() const;

private:
    QString m_fileName;
    QSettings m_settings;
};
