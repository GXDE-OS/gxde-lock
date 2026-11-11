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

#ifndef SRC_LOCK_COMMON_LOCKSTYLEMANAGER_H_
#define SRC_LOCK_COMMON_LOCKSTYLEMANAGER_H_

#include <QObject>

#include "lockstyle.h"

class SessionBaseModel;

namespace gxde {
namespace lock {

// Watches the configured lock screen style and asks the running locker to hand
// off to the matching binary. The style itself is persisted by the root helper
// (top.gxde.Lock.System.SetLockScreenStyle) because /etc/gxde-lock.conf is not
// writable from the session; this class only re-reads it.
//
// A change made while the lock screen is visible is deferred until unlock so
// the session lock is never released while the desktop is hidden.
class LockStyleManager : public QObject {
  Q_OBJECT

 public:
  explicit LockStyleManager(
      SessionBaseModel* model, LockStyle running,
      const LockStylePaths& paths = LockStyleResolver::DefaultPaths(),
      QObject* parent = nullptr);

  LockStyle RunningStyle() const;
  LockStyle ConfiguredStyle() const;
  bool HandoffPending() const;

 public slots:
  // Re-reads the configuration. Emits HandoffRequested() when the configured
  // style differs from the running one and the lock screen is hidden.
  void Refresh();

 signals:
  void HandoffRequested(const QString& executable);

 private slots:
  void onVisibleChanged(bool visible);

 private:
  SessionBaseModel* model_;
  LockStyle running_;
  LockStylePaths paths_;
  bool visible_ = false;
  bool handoff_pending_ = false;
};

}  // namespace lock
}  // namespace gxde

#endif  // SRC_LOCK_COMMON_LOCKSTYLEMANAGER_H_
