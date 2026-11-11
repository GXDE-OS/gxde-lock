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

#ifndef SRC_LOCK_COMMON_LOCKSTYLE_H_
#define SRC_LOCK_COMMON_LOCKSTYLE_H_

#include <QByteArray>
#include <QList>
#include <QString>

namespace gxde {
namespace lock {

enum class LockStyle {
  kClassic,
  kModern,
};

struct LockStylePaths {
  QString configFile;
  QString omgConfig;
  QString binDir;
};

// Resolves the configured lock screen style and the executable that implements
// it. gxde-lock.conf's [General] LockScreenStyle takes precedence over
// OMG.conf's [NextFeatReq] SwitchLkScrStyle; the default is modern, falling
// back to classic when gxde-lock-neo is not installed.
class LockStyleResolver {
 public:
  static LockStylePaths DefaultPaths();

  static LockStyle Resolve(const LockStylePaths& paths);
  static LockStyle ResolveFromFiles(const QString& configFile,
                                    const QString& omgConfig,
                                    const QString& binDir);

  static QString StyleName(LockStyle style);
  static QString ExecutableName(LockStyle style);
  static QString ExecutablePath(LockStyle style, const QString& binDir);

  static bool ParseStyleName(const QString& value, LockStyle* style);

 private:
  static QString ReadSetting(const QString& path, const QString& group,
                             const QString& key);
};

// Replaces the current process with `executable`, passing `arguments` (argv[0]
// is added automatically). Only returns, with false, when execv() failed.
// Callers must release single-instance locks and D-Bus names first.
bool ExecLockScreen(const QString& executable,
                    const QList<QByteArray>& arguments);

}  // namespace lock
}  // namespace gxde

#endif  // SRC_LOCK_COMMON_LOCKSTYLE_H_
