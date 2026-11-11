#ifndef SRC_COMMON_LOCKAPPEARANCE_H_
#define SRC_COMMON_LOCKAPPEARANCE_H_

#include <QString>

// Lock screen wallpaper and cursor settings, shared by gxde-lock-classic and
// gxde-lock-neo. The global values live in the root helper's state file.
namespace LockAppearance {

QString stateFilePath();
QString cursorTheme(const QString& statePath = QString());
QString wallpaper(const QString& statePath = QString());
bool hasConfiguredWallpaper(const QString& statePath = QString());
QString lockWallpaperOverridePath(
  uint uid, const QString& dataDirectory = QString());
QString lockWallpaperOverride(
  uint uid, const QString& dataDirectory = QString());
QString gxdeDefaultWallpaper();
QString ddeLockDefaultWallpaper();

// The local path of a readable image given as a path or a file:// URL, or an
// empty string.
QString readableImage(const QString& path);

// The user's own lock wallpaper: the gxde-lock override, then Deepin
// Accounts, then freedesktop AccountsService. Empty when none has one.
QString userLockWallpaper(const QString& userName, uint uid);

}  // namespace LockAppearance

#endif  // SRC_COMMON_LOCKAPPEARANCE_H_
