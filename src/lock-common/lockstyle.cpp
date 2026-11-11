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

#include "lockstyle.h"

#include "Constants.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <QtDebug>

#include <cerrno>
#include <cstring>
#include <vector>

#include <unistd.h>

namespace gxde {
namespace lock {

LockStylePaths LockStyleResolver::DefaultPaths() {
  LockStylePaths paths;
  paths.configFile = QStringLiteral(CONFIG_FILE);
  paths.omgConfig = QStringLiteral(OMG_CONFIG_FILE);
  paths.binDir = QCoreApplication::applicationDirPath();
  return paths;
}

LockStyle LockStyleResolver::Resolve(const LockStylePaths& paths) {
  LockStyle style = LockStyle::kModern;

  const QString config_value =
      ReadSetting(paths.configFile, QStringLiteral("General"),
                  QStringLiteral("LockScreenStyle"));
  if (!ParseStyleName(config_value, &style)) {
    const QString omg_value =
        ReadSetting(paths.omgConfig, QStringLiteral("NextFeatReq"),
                    QStringLiteral("SwitchLkScrStyle"));
    if (!ParseStyleName(omg_value, &style)) {
      style = LockStyle::kModern;
    }
  }

  // The classic locker is always installed; it is the fallback whenever the
  // configured binary is missing.
  if (!QFileInfo(ExecutablePath(style, paths.binDir)).isExecutable()) {
    style = LockStyle::kClassic;
  }
  return style;
}

LockStyle LockStyleResolver::ResolveFromFiles(const QString& configFile,
                                              const QString& omgConfig,
                                              const QString& binDir) {
  LockStylePaths paths;
  paths.configFile = configFile;
  paths.omgConfig = omgConfig;
  paths.binDir = binDir;
  return Resolve(paths);
}

QString LockStyleResolver::StyleName(LockStyle style) {
  switch (style) {
    case LockStyle::kModern:
      return QStringLiteral("modern");
    case LockStyle::kClassic:
    default:
      return QStringLiteral("classic");
  }
}

QString LockStyleResolver::ExecutableName(LockStyle style) {
  switch (style) {
    case LockStyle::kModern:
      return QStringLiteral("gxde-lock-neo");
    case LockStyle::kClassic:
    default:
      return QStringLiteral("gxde-lock-classic");
  }
}

QString LockStyleResolver::ExecutablePath(LockStyle style,
                                          const QString& binDir) {
  return QDir(binDir).filePath(ExecutableName(style));
}

bool LockStyleResolver::ParseStyleName(const QString& value,
                                       LockStyle* style) {
  if (!style) return false;

  const QString normalized = value.trimmed().toLower();
  if (normalized == QLatin1String("modern") ||
      normalized == QLatin1String("neo")) {
    *style = LockStyle::kModern;
    return true;
  }
  if (normalized == QLatin1String("classic")) {
    *style = LockStyle::kClassic;
    return true;
  }
  return false;
}

QString LockStyleResolver::ReadSetting(const QString& path,
                                       const QString& group,
                                       const QString& key) {
  if (path.isEmpty() || !QFileInfo::exists(path)) return QString();

  QSettings settings(path, QSettings::IniFormat);
  QString value;
  if (group == QLatin1String("General")) {
    // QSettings flattens the INI [General] section into the top level.
    value = settings.value(key).toString().trimmed();
  } else {
    settings.beginGroup(group);
    value = settings.value(key).toString().trimmed();
    settings.endGroup();
  }

  const int semicolon = value.indexOf(QLatin1Char(';'));
  if (semicolon >= 0) value = value.left(semicolon).trimmed();
  const int hash = value.indexOf(QLatin1Char('#'));
  if (hash >= 0) value = value.left(hash).trimmed();
  return value;
}

bool ExecLockScreen(const QString& executable,
                    const QList<QByteArray>& arguments) {
  QList<QByteArray> storage;
  storage.reserve(arguments.size() + 1);
  storage << QFile::encodeName(executable);
  storage << arguments;

  std::vector<char*> argv;
  argv.reserve(static_cast<size_t>(storage.size()) + 1);
  for (QByteArray& argument : storage) argv.push_back(argument.data());
  argv.push_back(nullptr);

  execv(argv.front(), argv.data());
  qWarning() << "Failed to exec" << executable << ":" << std::strerror(errno);
  return false;
}

}  // namespace lock
}  // namespace gxde
