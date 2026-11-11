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

#ifndef SRC_GUARD_BROKEN_NOTICE_H_
#define SRC_GUARD_BROKEN_NOTICE_H_

#include <memory>

#include <QHash>
#include <QObject>
#include <QString>

#include "LogindSession.h"

class QScreen;
class QSocketNotifier;
class WaylandSessionLockIntegration;

namespace gxde::lock {

class NoticeWindow;

// Holds the session lock after the lock screen kept crashing, the way Plasma
// does: no password prompt, only instructions to unlock from a text console
// with `loginctl unlock-session` (or SIGUSR1 to this process). Covers every
// output and follows hotplug like the lock screens.
class BrokenNotice : public QObject {
  Q_OBJECT

 public:
  explicit BrokenNotice(QObject* parent = nullptr);
  ~BrokenNotice() override;

  // False when there is nothing to hold: a Wayland compositor without
  // ext-session-lock-v1 drops the lock together with the crashed client.
  bool Start();

 private:
  QString Message() const;
  void AddScreen(QScreen* screen);
  void RemoveScreen(QScreen* screen);
  void GrabX11Input();
  void Finish();

  const bool use_x11_;
  bool finishing_ = false;
  int grab_attempts_ = 0;
  LogindSession logind_;
  std::unique_ptr<WaylandSessionLockIntegration> session_lock_;
  QHash<QScreen*, NoticeWindow*> windows_;
  QSocketNotifier* unlock_signal_ = nullptr;
};

}  // namespace gxde::lock

#endif  // SRC_GUARD_BROKEN_NOTICE_H_
