#ifndef SRC_LOCK_CLASSIC_LOCK_LOCKSHORTCUTMANAGER_H_
#define SRC_LOCK_CLASSIC_LOCK_LOCKSHORTCUTMANAGER_H_

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QList>
#include <QObject>

class QDBusServiceWatcher;
class SessionBaseModel;

struct DBusKeySequence {
  QList<int> keys;
};

using DBusKeySequenceList = QList<DBusKeySequence>;

Q_DECLARE_METATYPE(DBusKeySequence)
Q_DECLARE_METATYPE(DBusKeySequenceList)

QDBusArgument& operator<<(
  QDBusArgument& argument, const DBusKeySequence& sequence);
const QDBusArgument& operator>>(const QDBusArgument& argument,
  DBusKeySequence& sequence);

class LockShortcutManager : public QObject {
  Q_OBJECT

 public:
  explicit LockShortcutManager(
    SessionBaseModel* model, QObject* parent = nullptr);
  ~LockShortcutManager() override;

  bool tryEnroll(bool enabled);
  bool isRegistered() const;
  void showLock();
  void refreshLockWallpaper();

 signals:
  void registrationChanged(bool registered);

 private slots:
  void onGlobalShortcutPressed(const QString& component, const QString& action,
    qlonglong timestamp);
  void onShortcutsChanged(const QStringList& actionId,
    const DBusKeySequenceList& keys);
  void onKGlobalAccelOwnerChanged(const QString& service, const QString& oldOwner,
    const QString& newOwner);

 private:
  bool enroll();
  void withdraw(bool forget);
  bool connectComponentSignal();
  void setRegistered(bool registered);

  static QStringList actionId();
  static DBusKeySequence superLSequence();
  static bool containsSuperL(const DBusKeySequenceList& sequences);

  SessionBaseModel* m_model = nullptr;
  QDBusServiceWatcher* m_serviceWatcher = nullptr;
  QString m_componentPath;
  bool m_enrollmentRequested = false;
  bool m_registered = false;
};

// The public session-bus API of gxde-lock (top.gxde.Lock). Settings shared by
// every user are forwarded to the root helper on the system bus.
class GxdeLockService : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "top.gxde.Lock")
  Q_PROPERTY(bool LkScrStat READ LkScrStat NOTIFY LkScrStatChanged)
  Q_PROPERTY(QString LockWallpaperOverride READ LockWallpaperOverride)
  Q_PROPERTY(QString LockScreenStyle READ LockScreenStyle
                 NOTIFY LockScreenStyleChanged)

 public:
  explicit GxdeLockService(LockShortcutManager* manager);

 public slots:
  bool TryEnrollLkScr(bool enabled);
  void UnregisterLkScr();
  bool LkScrStat() const;
  bool SetCursor(const QString& theme);
  bool SetWallpaperGXDEDefault();
  bool SetWallpaperDDELockDefault();
  bool SetWallpaper(const QString& wallpaper);
  bool ClearWallpaper();
  bool SetLockWallpaperOverride(const QString& wallpaper);
  bool ClearLockWallpaperOverride();
  QString LockWallpaperOverride() const;
  // "classic" or "modern". Persisted by the root helper; the running locker
  // hands off to the matching binary the next time the screen is unlocked.
  bool SetLockScreenStyle(const QString& style);
  QString LockScreenStyle() const;
  void Show();

 signals:
  void LkScrStatChanged(bool registered);
  void LockScreenStyleChanged(const QString& style);

 private slots:
  void onSystemLockScreenStyleChanged(const QString& style);

 private:
  LockShortcutManager* m_manager = nullptr;
};

#endif  // SRC_LOCK_CLASSIC_LOCK_LOCKSHORTCUTMANAGER_H_
