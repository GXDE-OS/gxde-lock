#ifndef SYSTEMDEFAULTS_H
#define SYSTEMDEFAULTS_H

// OS defaults shared by gxde-lock-classic and gxde-lock-neo.

#include <QString>

namespace LockSystemDefaults {

QString operatingSystemId(const QString &osReleasePath = QString());
bool isGxdeOperatingSystem(const QString &osReleasePath = QString());
QString defaultSessionDesktopFile(const QString &osReleasePath = QString());
QString initialSessionDesktopFile(const QString &rememberedSession,
    const QString &osReleasePath = QString());

} // namespace LockSystemDefaults

#endif // SYSTEMDEFAULTS_H
