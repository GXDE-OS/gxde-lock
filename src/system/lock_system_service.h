#ifndef SRC_SYSTEM_LOCK_SYSTEM_SERVICE_H_
#define SRC_SYSTEM_LOCK_SYSTEM_SERVICE_H_

#include <QDBusConnection>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QString>

class QTimer;

namespace gxde {
namespace lock {

// Root-owned top.gxde.Lock.System service. It persists the settings every
// user's locker shares: the lock screen style in the main config file, and
// the global wallpaper and cursor theme in the state file. Unprivileged
// lockers call it through the session-bus top.gxde.Lock interface.
//
// The service is D-Bus activated and exits after a short idle period; the
// signals are only emitted from method calls, so nobody misses one.
class LockSystemService : public QObject {
  Q_OBJECT

 public:
  explicit LockSystemService(QObject* parent = nullptr);

  // Registers the service name and object on `connection`. Returns false
  // when either is already taken.
  bool Register(QDBusConnection connection);

 public slots:
  bool SetCursor(const QString& theme);
  bool SetWallpaperGXDEDefault();
  bool SetWallpaperDDELockDefault();
  bool SetWallpaper(const QDBusUnixFileDescriptor& wallpaper);
  bool ClearWallpaper();
  bool SetLockScreenStyle(const QString& style);
  QString LockScreenStyle() const;
  void PowerOff();
  void Reboot();
  void Suspend();
  void Hibernate();

 signals:
  void LockScreenStyleChanged(const QString& style);
  void WallpaperChanged(const QString& wallpaper);

 private:
  bool SaveWallpaperPath(const QString& path);
  void CallLogind(const QString& method, const QString& fallback_command);
  void KeepAlive() const;

  QTimer* idle_timer_ = nullptr;
};

}  // namespace lock
}  // namespace gxde

#endif  // SRC_SYSTEM_LOCK_SYSTEM_SERVICE_H_
