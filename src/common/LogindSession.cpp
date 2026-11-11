/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "LogindSession.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>

namespace gxde::lock {

namespace {

constexpr int kCallTimeoutMs = 2000;

const QString kService = QStringLiteral("org.freedesktop.login1");
const QString kManagerPath = QStringLiteral("/org/freedesktop/login1");
const QString kManagerInterface =
    QStringLiteral("org.freedesktop.login1.Manager");
const QString kSessionInterface =
    QStringLiteral("org.freedesktop.login1.Session");

QString SessionPath(const QString& id) {
  QDBusMessage call = QDBusMessage::createMethodCall(
      kService, kManagerPath, kManagerInterface, QStringLiteral("GetSession"));
  call << id;
  const QDBusReply<QDBusObjectPath> reply =
      QDBusConnection::systemBus().call(call, QDBus::Block, kCallTimeoutMs);
  return reply.isValid() ? reply.value().path() : QString();
}

QVariant SessionProperty(const QString& path, const QString& name) {
  QDBusMessage call = QDBusMessage::createMethodCall(
      kService, path, QStringLiteral("org.freedesktop.DBus.Properties"),
      QStringLiteral("Get"));
  call << kSessionInterface << name;
  const QDBusReply<QDBusVariant> reply =
      QDBusConnection::systemBus().call(call, QDBus::Block, kCallTimeoutMs);
  return reply.isValid() ? reply.value().variant() : QVariant();
}

}  // namespace

LogindSession::LogindSession(QObject* parent) : QObject(parent) {
  // XDG autostart entries often run outside the session scope (systemd user
  // units), so prefer the id the session exported; "auto" then falls back to
  // the caller's session or the user's graphical session.
  const QString exported = qEnvironmentVariable("XDG_SESSION_ID");
  if (!exported.isEmpty()) path_ = SessionPath(exported);
  if (path_.isEmpty()) path_ = SessionPath(QStringLiteral("auto"));
  if (path_.isEmpty()) {
    qWarning() << "(Lock) No logind session found; loginctl lock-session and"
                  " unlock-session will not reach the locker.";
    return;
  }

  id_ = SessionProperty(path_, QStringLiteral("Id")).toString();
  vt_number_ = SessionProperty(path_, QStringLiteral("VTNr")).toUInt();

  QDBusConnection bus = QDBusConnection::systemBus();
  bus.connect(kService, path_, kSessionInterface, QStringLiteral("Lock"), this,
              SLOT(OnLock()));
  bus.connect(kService, path_, kSessionInterface, QStringLiteral("Unlock"),
              this, SLOT(OnUnlock()));
  qInfo() << "(Lock) Watching logind session" << id_;
}

void LogindSession::SetLockedHint(bool locked) {
  if (!IsValid()) return;
  QDBusMessage call = QDBusMessage::createMethodCall(
      kService, path_, kSessionInterface, QStringLiteral("SetLockedHint"));
  call << locked;
  QDBusConnection::systemBus().asyncCall(call);
}

void LogindSession::OnLock() { Q_EMIT LockRequested(); }

void LogindSession::OnUnlock() { Q_EMIT UnlockRequested(); }

}  // namespace gxde::lock
