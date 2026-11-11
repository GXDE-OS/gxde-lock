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

#include <string.h>

#include <QGuiApplication>
#include <QStringList>

#include "Constants.h"
#include "broken_notice.h"
#include "lock_guard.h"

namespace {

// --notice: hold the session lock with the crash notice until the session is
// unlocked through logind or SIGUSR1. Exit status 0 means unlocked (or
// nothing to hold).
int RunNotice(int argc, char* argv[]) {
  QGuiApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("gxde-lock-guard"));
  app.setQuitOnLastWindowClosed(false);

  gxde::lock::BrokenNotice notice;
  if (!notice.Start()) return 0;
  return app.exec();
}

}  // namespace

// gxde-lock-guard: the session's entry point for the lock screen. It runs
// gxde-lock-classic with the given arguments and keeps a locker in place if
// it crashes; see LockGuard.
int main(int argc, char* argv[]) {
  if (argc > 1 && strcmp(argv[1], "--notice") == 0) return RunNotice(argc, argv);

  QStringList args;
  for (int i = 1; i < argc; ++i) args << QString::fromLocal8Bit(argv[i]);
  gxde::lock::LockGuard guard(
      QStringLiteral(BIN_INSTALL_DIR "/gxde-lock-classic"), args,
      QStringLiteral(LIBEXEC_INSTALL_DIR "/gxde-lock-guard"));
  return guard.Run();
}
