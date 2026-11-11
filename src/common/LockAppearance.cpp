#include "LockAppearance.h"

#include "Constants.h"

#include <pwd.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QSettings>
#include <QStringList>
#include <QUrl>

namespace {

QString resolvedStatePath(const QString& statePath) {
  return statePath.isEmpty() ? LockAppearance::stateFilePath()
                             : statePath;
}

bool isReadableFile(const QString& path) {
  QFile file(path);
  return file.open(QIODevice::ReadOnly);
}

// GreeterBackground, falling back to BackgroundFile, and finally to the
// user's desktop wallpaper list, of an Accounts user object. The Deepin
// daemon is the only one exposing DesktopBackgrounds, which mirrors the
// GSettings desktop wallpaper the in-session locker reads; keeping it here
// lets the classic locker resolve the very same image as the desktop.
QString accountsUserBackground(const QString& service, const QString& path,
    const QString& interface) {
  QDBusInterface user(service, path, interface,
    QDBusConnection::systemBus());
  if (!user.isValid()) return QString();

  const QStringList properties = {
    QStringLiteral("GreeterBackground"),
    QStringLiteral("BackgroundFile"),
  };
  for (const QString& property : properties) {
    const QByteArray name = property.toLatin1();
    const QString background =
      LockAppearance::readableImage(user.property(name.constData()).toString());
    if (!background.isEmpty()) return background;
  }

  const QStringList desktopBackgrounds =
    user.property("DesktopBackgrounds").toStringList();
  for (const QString& desktopBackground : desktopBackgrounds) {
    const QString background =
      LockAppearance::readableImage(desktopBackground);
    if (!background.isEmpty()) return background;
  }
  return QString();
}

QString deepinAccountsLockBackground(const QString& userName) {
  QDBusInterface accounts(QStringLiteral("com.deepin.daemon.Accounts"),
    QStringLiteral("/com/deepin/daemon/Accounts"),
    QStringLiteral("com.deepin.daemon.Accounts"),
    QDBusConnection::systemBus());
  if (!accounts.isValid()) return QString();

  const QDBusReply<QString> userPath = accounts.call(
    QStringLiteral("FindUserByName"), userName);
  if (!userPath.isValid() || userPath.value().isEmpty()) return QString();

  return accountsUserBackground(QStringLiteral("com.deepin.daemon.Accounts"),
    userPath.value(), QStringLiteral("com.deepin.daemon.Accounts.User"));
}

QString accountsServiceLockBackground(const QString& userName) {
  // org.freedesktop.Accounts is a system service, so it is reachable even
  // without com.deepin.daemon.Accounts.
  QDBusInterface accounts(QStringLiteral("org.freedesktop.Accounts"),
    QStringLiteral("/org/freedesktop/Accounts"),
    QStringLiteral("org.freedesktop.Accounts"),
    QDBusConnection::systemBus());
  if (!accounts.isValid()) return QString();

  const QDBusReply<QDBusObjectPath> userPath = accounts.call(
    QStringLiteral("FindUserByName"), userName);
  if (!userPath.isValid() || userPath.value().path().isEmpty()) {
    return QString();
  }

  return accountsUserBackground(QStringLiteral("org.freedesktop.Accounts"),
    userPath.value().path(), QStringLiteral("org.freedesktop.Accounts.User"));
}

}  // namespace

namespace LockAppearance {

QString stateFilePath() {
  return QStringLiteral(STATE_DIR "/state.conf");
}

QString cursorTheme(const QString& statePath) {
  const QSettings settings(resolvedStatePath(statePath), QSettings::IniFormat);
  return settings.value(QStringLiteral("Lock/CursorTheme"))
    .toString()
    .trimmed();
}

QString wallpaper(const QString& statePath) {
  const QSettings settings(resolvedStatePath(statePath), QSettings::IniFormat);
  const QString configured =
    settings.value(QStringLiteral("Lock/Wallpaper"), gxdeDefaultWallpaper())
      .toString();

  if (isReadableFile(configured)) return configured;
  if (isReadableFile(gxdeDefaultWallpaper())) return gxdeDefaultWallpaper();
  return ddeLockDefaultWallpaper();
}

bool hasConfiguredWallpaper(const QString& statePath) {
  // state.conf 中是否显式写入过 [Lock] Wallpaper。
  // 值为空（ClearWallpaper 写入 "Wallpaper="）视为未配置，
  // 未写入时调用方应回退到用户自己的壁纸，而不是全局默认图。
  const QSettings settings(resolvedStatePath(statePath), QSettings::IniFormat);
  const QString configured = settings.value(
    QStringLiteral("Lock/Wallpaper"), QString()).toString().trimmed();
  return !configured.isEmpty();
}

QString lockWallpaperOverridePath(uint uid, const QString& dataDirectory) {
  QString directory = dataDirectory;
  if (directory.isEmpty()) {
    const passwd* user = getpwuid(uid);
    if (!user || !user->pw_dir) return QString();
    directory = QString::fromLocal8Bit(user->pw_dir)
      + QStringLiteral("/.local/share/gxde-lock");
  }
  return directory + QStringLiteral("/lock-wallpaper-override");
}

QString lockWallpaperOverride(uint uid, const QString& dataDirectory) {
  const QString configured = lockWallpaperOverridePath(uid, dataDirectory);

  return isReadableFile(configured) ? configured : QString();
}

QString gxdeDefaultWallpaper() {
  return QStringLiteral("/usr/share/backgrounds/default_background.jpg");
}

QString ddeLockDefaultWallpaper() {
  return QStringLiteral(":/theme/background/default_background.jpg");
}

QString readableImage(const QString& path) {
  const QUrl url(path);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : path;
  const QFileInfo file(localPath);
  if (!file.isFile() || !file.isReadable()) return QString();

  QImageReader reader(localPath);
  return reader.canRead() ? localPath : QString();
}

QString userLockWallpaper(const QString& userName, uint uid) {
  // 1. The gxde-lock override (~/.local/share/gxde-lock) belongs to one user
  //    and never affects the global wallpaper.
  const QString override = readableImage(lockWallpaperOverride(uid));
  if (!override.isEmpty()) {
    qInfo() << "(Frontend) Lock background: Using user override for"
      << userName << override;
    return override;
  }

  // 2. Deepin Accounts, the interface the classic lightdm greeter used for
  //    the login wallpaper.
  const QString accountsBackground = deepinAccountsLockBackground(userName);
  if (!accountsBackground.isEmpty()) {
    qInfo() << "(Frontend) Lock background: Using Deepin Accounts for"
      << userName << accountsBackground;
    return accountsBackground;
  }

  // 3. Freedesktop AccountsService for systems without the Deepin daemon.
  const QString freedesktopBackground =
    accountsServiceLockBackground(userName);
  if (!freedesktopBackground.isEmpty()) {
    qInfo() << "(Frontend) Lock background: Using AccountsService for"
      << userName << freedesktopBackground;
  }
  return freedesktopBackground;
}

}  // namespace LockAppearance
