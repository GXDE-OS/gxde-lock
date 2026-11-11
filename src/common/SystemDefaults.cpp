#include "SystemDefaults.h"

#include <QFile>
#include <QSettings>

namespace LockSystemDefaults {

QString operatingSystemId(const QString &osReleasePath)
{
    QString path = osReleasePath;
    if (path.isEmpty()) {
        path = QStringLiteral("/etc/os-release");
        if (!QFile::exists(path))
            path = QStringLiteral("/usr/lib/os-release");
    }

    QSettings osRelease(path, QSettings::IniFormat);
    return osRelease.value(QStringLiteral("ID")).toString();
}

bool isGxdeOperatingSystem(const QString& osReleasePath) {
    return operatingSystemId(osReleasePath)
        .compare(QStringLiteral("gxde"), Qt::CaseInsensitive) == 0;
}

QString defaultSessionDesktopFile(const QString &osReleasePath)
{
    if (isGxdeOperatingSystem(osReleasePath)) {
        return QStringLiteral("deepin.desktop");
    }

    return QString();
}

QString initialSessionDesktopFile(const QString &rememberedSession,
    const QString &osReleasePath)
{
    if (!rememberedSession.isEmpty())
        return rememberedSession;

    return defaultSessionDesktopFile(osReleasePath);
}

} // namespace LockSystemDefaults
