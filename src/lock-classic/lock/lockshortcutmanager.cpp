#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QKeyCombination>
#include <QSaveFile>
#include <QTimer>
#include <QUrl>

#include <unistd.h>

#include "LockAppearance.h"
#include "sessionbasemodel.h"
#include "lockshortcutmanager.h"

namespace {

const QString kKGlobalAccelService = QStringLiteral("org.kde.kglobalaccel");
const QString kKGlobalAccelPath = QStringLiteral("/kglobalaccel");
const QString kKGlobalAccelInterface = QStringLiteral("org.kde.KGlobalAccel");
const QString kKGlobalAccelComponentInterface = QStringLiteral(
  "org.kde.kglobalaccel.Component");
const QString kComponent = QStringLiteral("gxde-lock");
const QString kAction = QStringLiteral("lock-screen");
const QString kSystemService = QStringLiteral("top.gxde.Lock");
const QString kSystemPath = QStringLiteral("/top/gxde/Lock");
const QString kSystemInterface = QStringLiteral("top.gxde.Lock.System");

// GXDM's session settings agent. The greeter shows each user's lock wallpaper
// on the login screen, but runs as another user and cannot read ours, so we
// push a copy there. Optional: nothing happens without GXDM installed.
const QString kGreeterSettingsService =
  QStringLiteral("top.gxde.DisplayManager");
const QString kGreeterSettingsPath = QStringLiteral("/top/gxde/DisplayManager");
const QString kGreeterSettingsInterface =
  QStringLiteral("top.gxde.DisplayManager");

constexpr uint kSetPresent = 2;
constexpr uint kNoAutoloading = 4;
constexpr qint64 kMaxWallpaperSize = 128 * 1024 * 1024;

bool callSystemHelper(const QString& method,
    const QVariantList& arguments = {}) {
  QDBusInterface interface(kSystemService,
    kSystemPath, kSystemInterface,
    QDBusConnection::systemBus());
  const QDBusReply<bool> reply(interface.callWithArgumentList(
    QDBus::Block, method, arguments));
  if (!reply.isValid()) {
    qWarning() << method << "failed:" << reply.error().message();
    return false;
  }
  return reply.value();
}

QString querySystemHelper(const QString& method,
    const QVariantList& arguments = {}) {
  QDBusInterface interface(kSystemService,
    kSystemPath, kSystemInterface,
    QDBusConnection::systemBus());
  const QDBusReply<QString> reply(interface.callWithArgumentList(
    QDBus::Block, method, arguments));
  if (!reply.isValid()) {
    return QString();
  }
  return reply.value();
}

bool sendWallpaperFileToSystem(const QString& method, const QString& localPath,
    QVariantList arguments = {}) {
  QImageReader reader(localPath);
  if (!reader.canRead()) return false;

  QFile file(localPath);
  if (!file.open(QIODevice::ReadOnly)) return false;

  const QDBusUnixFileDescriptor descriptor(file.handle());
  if (!descriptor.isValid()) return false;

  arguments << QVariant::fromValue(descriptor);
  return callSystemHelper(method, arguments);
}

bool saveCurrentUserLockWallpaper(const QString& localPath) {
  QImageReader reader(localPath);
  if (!reader.canRead()) return false;

  QFile source(localPath);
  if (!source.open(QIODevice::ReadOnly)) return false;

  const QString targetPath =
    LockAppearance::lockWallpaperOverridePath(getuid());
  if (targetPath.isEmpty()
      || !QDir().mkpath(QFileInfo(targetPath).absolutePath())) {
    return false;
  }

  QSaveFile target(targetPath);
  if (!target.open(QIODevice::WriteOnly)) return false;

  qint64 total = 0;
  while (!source.atEnd()) {
    const QByteArray chunk = source.read(1024 * 1024);
    if (chunk.isEmpty() && source.error() != QFileDevice::NoError) {
      target.cancelWriting();
      return false;
    }
    total += chunk.size();
    if (total > kMaxWallpaperSize
        || target.write(chunk) != chunk.size()) {
      target.cancelWriting();
      return false;
    }
  }

  if (total == 0 || !target.commit()) return false;
  return QFile::setPermissions(targetPath,
    QFileDevice::ReadOwner | QFileDevice::WriteOwner
    | QFileDevice::ReadGroup | QFileDevice::ReadOther);
}

// Fire and forget: the lock screen must not wait on the display manager.
void pushOverrideToGreeter(const QString& method,
    const QVariantList& arguments = {}) {
  QDBusMessage call = QDBusMessage::createMethodCall(kGreeterSettingsService,
    kGreeterSettingsPath, kGreeterSettingsInterface, method);
  call.setArguments(arguments);
  QDBusConnection::sessionBus().call(call, QDBus::NoBlock);
}

}  // namespace

QDBusArgument& operator<<(
    QDBusArgument& argument, const DBusKeySequence& sequence) {
  argument.beginStructure();
  argument << sequence.keys;
  argument.endStructure();
  return argument;
}

const QDBusArgument& operator>>(
    const QDBusArgument& argument, DBusKeySequence& sequence) {
  argument.beginStructure();
  argument >> sequence.keys;
  argument.endStructure();
  return argument;
}

LockShortcutManager::LockShortcutManager(
    SessionBaseModel* model, QObject* parent)
    : QObject(parent),
      m_model(model),
      m_serviceWatcher(new QDBusServiceWatcher(kKGlobalAccelService,
          QDBusConnection::sessionBus(),
          QDBusServiceWatcher::WatchForOwnerChange, this)) {
  qDBusRegisterMetaType<DBusKeySequence>();
  qDBusRegisterMetaType<DBusKeySequenceList>();

  connect(m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
    &LockShortcutManager::onKGlobalAccelOwnerChanged);

  QDBusConnection::sessionBus().connect(kKGlobalAccelService, kKGlobalAccelPath,
    kKGlobalAccelInterface, QStringLiteral("yourShortcutsChanged"), this,
    SLOT(onShortcutsChanged(QStringList, DBusKeySequenceList)));
}

LockShortcutManager::~LockShortcutManager() {
  if (m_registered) withdraw(false);
}

bool LockShortcutManager::tryEnroll(bool enabled) {
  m_enrollmentRequested = enabled;
  if (!enabled) {
    withdraw(true);
    return false;
  }

  return enroll();
}

bool LockShortcutManager::isRegistered() const { return m_registered; }

void LockShortcutManager::showLock() {
  if (m_model) m_model->setIsShow(true);
}

void LockShortcutManager::refreshLockWallpaper() {
  if (m_model && m_model->currentUser()) {
    m_model->currentUser()->refreshLockBackgroundPath();
  }
}

void LockShortcutManager::onGlobalShortcutPressed(
    const QString& component, const QString& action, qlonglong timestamp) {
  Q_UNUSED(timestamp)
  if (m_enrollmentRequested && m_registered
      && component == kComponent && action == kAction) {
    showLock();
  }
}

void LockShortcutManager::onShortcutsChanged(
    const QStringList& changedActionId, const DBusKeySequenceList& keys) {
  if (m_enrollmentRequested && changedActionId == actionId()) {
    setRegistered(containsSuperL(keys));
  }
}

void LockShortcutManager::onKGlobalAccelOwnerChanged(
    const QString& service, const QString& oldOwner, const QString& newOwner) {
  Q_UNUSED(service)
  Q_UNUSED(oldOwner)

  m_componentPath.clear();
  setRegistered(false);
  if (m_enrollmentRequested && !newOwner.isEmpty()) {
    QTimer::singleShot(0, this, [this] {
      enroll();
    });
  }
}

bool LockShortcutManager::enroll() {
  if (!m_enrollmentRequested) {
    return false;
  }

  QDBusInterface interface(kKGlobalAccelService, kKGlobalAccelPath,
      kKGlobalAccelInterface, QDBusConnection::sessionBus());

  const QDBusMessage registerReply =
    interface.call(QStringLiteral("doRegister"), actionId());
  if (registerReply.type() == QDBusMessage::ErrorMessage) {
    qWarning() << "KGlobalAccel registration failed:"
      << registerReply.errorMessage();
    setRegistered(false);
    return false;
  }

  const DBusKeySequenceList requested{superLSequence()};
  const QDBusReply<DBusKeySequenceList> shortcutReply =
      interface.call(QStringLiteral("setShortcutKeys"), actionId(),
          QVariant::fromValue(requested), kSetPresent | kNoAutoloading);
  if (!shortcutReply.isValid()) {
    qWarning() << "Setting Super+L failed:"
      << shortcutReply.error().message();
    withdraw(true);
    return false;
  }

  const bool registered = containsSuperL(shortcutReply.value());
  if (!registered) {
    qInfo() << "Super+L is already registered;"
      " leaving the existing shortcut untouched.";
    withdraw(true);
    return false;
  }

  if (!connectComponentSignal()) {
    qWarning() << "Failed to subscribe to activations.";
    withdraw(true);
    return false;
  }

  setRegistered(true);
  qInfo() << "Registered Super+L.";
  return true;
}

void LockShortcutManager::withdraw(bool forget) {
  QDBusInterface interface(kKGlobalAccelService, kKGlobalAccelPath,
    kKGlobalAccelInterface, QDBusConnection::sessionBus());

  if (forget) {
    interface.call(QStringLiteral("unregister"), kComponent, kAction);
  } else {
    interface.call(QStringLiteral("setInactive"), actionId());
  }

  if (!m_componentPath.isEmpty()) {
    QDBusConnection::sessionBus().disconnect(kKGlobalAccelService,
      m_componentPath, kKGlobalAccelComponentInterface,
      QStringLiteral("globalShortcutPressed"), this,
      SLOT(onGlobalShortcutPressed(QString, QString, qlonglong)));
    m_componentPath.clear();
  }
  setRegistered(false);
}

bool LockShortcutManager::connectComponentSignal() {
  QDBusInterface interface(kKGlobalAccelService, kKGlobalAccelPath,
    kKGlobalAccelInterface, QDBusConnection::sessionBus());
  const QDBusReply<QDBusObjectPath> componentReply =
    interface.call(QStringLiteral("getComponent"), kComponent);
  if (!componentReply.isValid()) return false;

  m_componentPath = componentReply.value().path();
  return QDBusConnection::sessionBus().connect(kKGlobalAccelService,
    m_componentPath, kKGlobalAccelComponentInterface,
    QStringLiteral("globalShortcutPressed"), this,
    SLOT(onGlobalShortcutPressed(QString, QString, qlonglong)));
}

void LockShortcutManager::setRegistered(bool registered) {
  if (m_registered == registered) return;

  m_registered = registered;
  emit registrationChanged(registered);
}

QStringList LockShortcutManager::actionId() {
  return {
    kComponent,
    kAction,
    QStringLiteral("GXDE Lock"),
    QStringLiteral("Lock Screen"),
  };
}

DBusKeySequence LockShortcutManager::superLSequence() {
  DBusKeySequence sequence;
  sequence.keys = {
    QKeyCombination(Qt::MetaModifier, Qt::Key_L).toCombined(),
    0,
    0,
    0,
  };
  return sequence;
}

bool LockShortcutManager::containsSuperL(const DBusKeySequenceList& sequences) {
  const int superL = superLSequence().keys.constFirst();
  for (const DBusKeySequence& sequence : sequences) {
    if (!sequence.keys.isEmpty() && sequence.keys.constFirst() == superL)
      return true;
  }
  return false;
}

GxdeLockService::GxdeLockService(
    LockShortcutManager* manager)
    : QDBusAbstractAdaptor(manager), m_manager(manager) {
  connect(manager, &LockShortcutManager::registrationChanged, this,
    &GxdeLockService::LkScrStatChanged);
  QDBusConnection::systemBus().connect(kSystemService,
    kSystemPath, kSystemInterface,
    QStringLiteral("LockScreenStyleChanged"), this,
    SLOT(onSystemLockScreenStyleChanged(QString)));
}

bool GxdeLockService::TryEnrollLkScr(bool enabled) {
  return m_manager->tryEnroll(enabled);
}

void GxdeLockService::UnregisterLkScr() {
  m_manager->tryEnroll(false);
}

bool GxdeLockService::LkScrStat() const {
  return m_manager->isRegistered();
}

bool GxdeLockService::SetCursor(const QString& theme) {
  return callSystemHelper(QStringLiteral("SetCursor"), {theme});
}

bool GxdeLockService::SetWallpaperGXDEDefault() {
  return callSystemHelper(QStringLiteral("SetWallpaperGXDEDefault"));
}

bool GxdeLockService::SetWallpaperDDELockDefault() {
  return callSystemHelper(
    QStringLiteral("SetWallpaperDDELockDefault"));
}

bool GxdeLockService::SetWallpaper(const QString& wallpaper) {
  const QUrl url(wallpaper);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : wallpaper;
  return sendWallpaperFileToSystem(QStringLiteral("SetWallpaper"), localPath);
}

bool GxdeLockService::ClearWallpaper() {
  return callSystemHelper(QStringLiteral("ClearWallpaper"));
}

bool GxdeLockService::SetLockWallpaperOverride(
    const QString& wallpaper) {
  const QUrl url(wallpaper);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : wallpaper;
  if (!saveCurrentUserLockWallpaper(localPath)) return false;

  pushOverrideToGreeter(QStringLiteral("SetLockWallpaperOverride"),
    {LockAppearance::lockWallpaperOverridePath(getuid())});
  m_manager->refreshLockWallpaper();
  return true;
}

bool GxdeLockService::ClearLockWallpaperOverride() {
  const QString path =
    LockAppearance::lockWallpaperOverridePath(getuid());
  if (path.isEmpty() || (QFileInfo::exists(path) && !QFile::remove(path))) {
    return false;
  }

  pushOverrideToGreeter(QStringLiteral("ClearLockWallpaperOverride"));
  m_manager->refreshLockWallpaper();
  return true;
}

QString GxdeLockService::LockWallpaperOverride() const {
  return LockAppearance::lockWallpaperOverride(getuid());
}

bool GxdeLockService::SetLockScreenStyle(const QString& style) {
  return callSystemHelper(QStringLiteral("SetLockScreenStyle"),
                                  {style});
}

QString GxdeLockService::LockScreenStyle() const {
  const QString style =
    querySystemHelper(QStringLiteral("LockScreenStyle"));
  return style.isEmpty() ? QStringLiteral("classic") : style;
}

void GxdeLockService::onSystemLockScreenStyleChanged(
    const QString& style) {
  emit LockScreenStyleChanged(style);
}

void GxdeLockService::Show() { m_manager->showLock(); }
