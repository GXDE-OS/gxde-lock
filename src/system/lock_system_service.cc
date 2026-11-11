#include "lock_system_service.h"

#include <unistd.h>

#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

#include "Constants.h"
#include "general_setting.h"
#include "lockstyle.h"
#include "locksystemadaptor.h"

namespace gxde {
namespace lock {
namespace {

const QString kService = QStringLiteral("top.gxde.Lock");
const QString kPath = QStringLiteral("/top/gxde/Lock");
const QString kGxdeDefaultWallpaper =
    QStringLiteral("/usr/share/backgrounds/default_background.jpg");
// Compiled into both lockers' resources.
const QString kDdeLockDefaultWallpaper =
    QStringLiteral(":/theme/background/default_background.jpg");
const QString kWallpaperKey = QStringLiteral("Lock/Wallpaper");
const QString kCursorThemeKey = QStringLiteral("Lock/CursorTheme");
const QString kWallpaperFileName = QStringLiteral("wallpaper");
constexpr qint64 kMaxWallpaperSize = 128 * 1024 * 1024;
constexpr int kIdleTimeoutMs = 60 * 1000;

QString StatePath() { return QStringLiteral(STATE_DIR "/state.conf"); }

bool CursorThemeExists(const QString& theme) {
  static const QRegularExpression valid_name(
      QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._+-]*$"));
  if (!valid_name.match(theme).hasMatch()) return false;

  return !QStandardPaths::locate(
              QStandardPaths::GenericDataLocation,
              QStringLiteral("icons/%1/index.theme").arg(theme),
              QStandardPaths::LocateFile)
              .isEmpty();
}

bool SaveState(const QString& key, const QString& value) {
  if (!QDir().mkpath(QStringLiteral(STATE_DIR))) return false;
  QSettings settings(StatePath(), QSettings::IniFormat);
  settings.setValue(key, value);
  settings.sync();
  return settings.status() == QSettings::NoError;
}

// Copies the image behind `wallpaper` into the state directory, so the
// wallpaper keeps working after the caller's original file goes away.
QString CopyWallpaper(const QDBusUnixFileDescriptor& wallpaper) {
  if (!wallpaper.isValid()) return QString();

  const int source_fd = dup(wallpaper.fileDescriptor());
  if (source_fd < 0) return QString();

  QFile source;
  if (!source.open(source_fd, QIODevice::ReadOnly,
                   QFileDevice::AutoCloseHandle)) {
    close(source_fd);
    return QString();
  }

  if (!QDir().mkpath(QStringLiteral(STATE_DIR))) return QString();

  const QString target_path =
      QStringLiteral(STATE_DIR) + QLatin1Char('/') + kWallpaperFileName;
  QSaveFile target(target_path);
  if (!target.open(QIODevice::WriteOnly)) return QString();

  qint64 total = 0;
  while (!source.atEnd()) {
    const QByteArray chunk = source.read(1024 * 1024);
    if (chunk.isEmpty() && source.error() != QFileDevice::NoError) {
      target.cancelWriting();
      return QString();
    }
    total += chunk.size();
    if (total > kMaxWallpaperSize || target.write(chunk) != chunk.size()) {
      target.cancelWriting();
      return QString();
    }
  }

  if (total == 0 || !target.commit()) return QString();

  if (!QFile::setPermissions(target_path,
                             QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                 QFileDevice::ReadGroup |
                                 QFileDevice::ReadOther)) {
    return QString();
  }
  return target_path;
}

LockStylePaths StylePaths() {
  LockStylePaths paths = LockStyleResolver::DefaultPaths();
  // This helper lives in libexec; the lockers are in the bin directory.
  paths.binDir = QStringLiteral(BIN_INSTALL_DIR);
  return paths;
}

}  // namespace

LockSystemService::LockSystemService(QObject* parent)
    : QObject(parent), idle_timer_(new QTimer(this)) {
  new SystemAdaptor(this);

  idle_timer_->setSingleShot(true);
  idle_timer_->setInterval(kIdleTimeoutMs);
  connect(idle_timer_, &QTimer::timeout, this, [] {
    qInfo() << "Idle, exiting";
    QCoreApplication::quit();
  });
  idle_timer_->start();
}

bool LockSystemService::Register(QDBusConnection connection) {
  if (!connection.registerObject(kPath, this)) {
    qWarning() << "Failed to register" << kPath << connection.lastError();
    return false;
  }
  if (!connection.registerService(kService)) {
    qWarning() << "Failed to register" << kService << connection.lastError();
    return false;
  }
  return true;
}

bool LockSystemService::SetCursor(const QString& theme) {
  KeepAlive();
  if (!CursorThemeExists(theme)) return false;
  return SaveState(kCursorThemeKey, theme);
}

bool LockSystemService::SetWallpaperGXDEDefault() {
  KeepAlive();
  if (!SaveWallpaperPath(kGxdeDefaultWallpaper)) return false;
  emit WallpaperChanged(kGxdeDefaultWallpaper);
  return true;
}

bool LockSystemService::SetWallpaperDDELockDefault() {
  KeepAlive();
  if (!SaveWallpaperPath(kDdeLockDefaultWallpaper)) return false;
  emit WallpaperChanged(kDdeLockDefaultWallpaper);
  return true;
}

bool LockSystemService::SetWallpaper(
    const QDBusUnixFileDescriptor& wallpaper) {
  KeepAlive();
  const QString target_path = CopyWallpaper(wallpaper);
  if (target_path.isEmpty() || !SaveWallpaperPath(target_path)) return false;
  emit WallpaperChanged(target_path);
  return true;
}

bool LockSystemService::ClearWallpaper() {
  KeepAlive();
  // An empty value means "no global wallpaper": each locker falls back to the
  // user's own wallpaper chain.
  if (!SaveWallpaperPath(QString())) return false;
  // Best effort; a stale copy is harmless once nothing points at it.
  QFile::remove(QStringLiteral(STATE_DIR) + QLatin1Char('/') +
                kWallpaperFileName);
  emit WallpaperChanged(QString());
  return true;
}

bool LockSystemService::SetLockScreenStyle(const QString& style) {
  KeepAlive();
  LockStyle requested = LockStyle::kClassic;
  if (!LockStyleResolver::ParseStyleName(style, &requested)) return false;

  // Refuse a style whose binary is not installed; the lockers would silently
  // fall back to classic and the setting would lie.
  const LockStylePaths paths = StylePaths();
  if (!QFileInfo(LockStyleResolver::ExecutablePath(requested, paths.binDir))
           .isExecutable()) {
    return false;
  }

  const QString before = LockScreenStyle();
  const QString normalized = LockStyleResolver::StyleName(requested);
  if (!gxde::SaveGeneralSetting(paths.configFile,
                                QStringLiteral("LockScreenStyle"),
                                normalized)) {
    return false;
  }

  if (LockScreenStyle() != before) emit LockScreenStyleChanged(normalized);
  return true;
}

QString LockSystemService::LockScreenStyle() const {
  KeepAlive();
  return LockStyleResolver::StyleName(LockStyleResolver::Resolve(StylePaths()));
}

void LockSystemService::PowerOff() {
  CallLogind(QStringLiteral("PowerOff"), QStringLiteral(HALT_COMMAND));
}

void LockSystemService::Reboot() {
  CallLogind(QStringLiteral("Reboot"), QStringLiteral(REBOOT_COMMAND));
}

void LockSystemService::Suspend() {
  CallLogind(QStringLiteral("Suspend"), QString());
}

void LockSystemService::Hibernate() {
  CallLogind(QStringLiteral("Hibernate"), QString());
}

bool LockSystemService::SaveWallpaperPath(const QString& path) {
  return SaveState(kWallpaperKey, path);
}

void LockSystemService::CallLogind(const QString& method,
                                   const QString& fallback_command) {
  KeepAlive();
  qInfo() << method << "requested by a locker";

  QDBusInterface login1(QStringLiteral("org.freedesktop.login1"),
                        QStringLiteral("/org/freedesktop/login1"),
                        QStringLiteral("org.freedesktop.login1.Manager"),
                        QDBusConnection::systemBus());
  if (login1.isValid()) {
    const QDBusMessage reply = login1.call(method, false);
    if (reply.type() != QDBusMessage::ErrorMessage) return;
    qWarning() << "logind" << method << "failed:" << reply.errorMessage();
  }

  if (fallback_command.isEmpty()) return;
  QStringList arguments = QProcess::splitCommand(fallback_command);
  if (arguments.isEmpty()) return;
  const QString program = arguments.takeFirst();
  QProcess::startDetached(program, arguments);
}

void LockSystemService::KeepAlive() const { idle_timer_->start(); }

}  // namespace lock
}  // namespace gxde
