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

#include "LockState.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace gxde::lock {

QString LockStateFile() {
  QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
  if (runtime.isEmpty()) {
    runtime =
        QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
  }
  // XDG_RUNTIME_DIR is per user, not per session; keep two graphical
  // sessions of one user apart by their display.
  QString display = qEnvironmentVariable("WAYLAND_DISPLAY");
  if (display.isEmpty()) display = qEnvironmentVariable("DISPLAY");
  display.replace(QLatin1Char('/'), QLatin1Char('_'));
  QString path = runtime + QStringLiteral("/gxde-lock/locked");
  if (!display.isEmpty()) path += QLatin1Char('-') + display;
  return path;
}

void MarkSessionLocked(bool locked) {
  const QString path = LockStateFile();
  if (!locked) {
    QFile::remove(path);
    return;
  }

  QDir().mkpath(path.section(QLatin1Char('/'), 0, -2));
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qWarning() << "(Lock) Cannot record the lock state in" << path << ":"
               << file.errorString();
  }
}

bool IsSessionMarkedLocked() { return QFile::exists(LockStateFile()); }

}  // namespace gxde::lock
