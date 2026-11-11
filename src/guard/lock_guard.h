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

#ifndef SRC_GUARD_LOCK_GUARD_H_
#define SRC_GUARD_LOCK_GUARD_H_

#include <sys/types.h>

#include <QElapsedTimer>
#include <QList>
#include <QString>
#include <QStringList>

namespace gxde::lock {

// Keeps a locker running for the session. A locker that dies while the
// screen is locked would leave a Wayland session locked by nobody (the
// compositor keeps it locked and nothing can unlock it), so the guard starts
// a replacement that takes the lock over at once. When the locker keeps
// crashing, the guard holds the lock with a minimal notice instead, which
// tells the user how to unlock from a text console.
class LockGuard {
 public:
  // |locker| is started with |args|; |notice| is the command that shows the
  // crash notice (this binary with --notice).
  LockGuard(QString locker, QStringList args, QString notice);

  // Returns once the locker exits on its own or the guard is told to stop.
  int Run();

 private:
  enum class Mode { kAsRequested, kDaemon, kLocked, kNotice };

  QStringList CommandFor(Mode mode) const;
  pid_t Spawn(const QStringList& command);
  int WaitFor(pid_t pid);
  // Records a crash; true once crashes come too fast to keep restarting.
  bool CrashLoop();

  const QString locker_;
  const QStringList args_;
  const QString notice_;
  QElapsedTimer clock_;
  QList<qint64> crashes_ms_;
};

}  // namespace gxde::lock

#endif  // SRC_GUARD_LOCK_GUARD_H_
