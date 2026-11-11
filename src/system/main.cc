#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusUnixFileDescriptor>

#include "lock_system_service.h"

// gxde-lock-system: the root-owned half of gxde-lock, activated on demand by
// the system bus. Pass --session-bus to run it unprivileged on the session
// bus for testing.
int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("gxde-lock-system"));
  qDBusRegisterMetaType<QDBusUnixFileDescriptor>();

  const bool session_bus =
      app.arguments().contains(QStringLiteral("--session-bus"));
  gxde::lock::LockSystemService service;
  if (!service.Register(session_bus ? QDBusConnection::sessionBus()
                                    : QDBusConnection::systemBus())) {
    return 1;
  }
  return app.exec();
}
