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

#ifndef SRC_COMMON_LOGINDSESSION_H_
#define SRC_COMMON_LOGINDSESSION_H_

#include <QObject>
#include <QString>

namespace gxde::lock {

// The logind session this process belongs to. `loginctl lock-session` and
// `loginctl unlock-session` arrive here as LockRequested/UnlockRequested;
// logind only lets the session's owner (or an administrator) send them.
class LogindSession : public QObject {
  Q_OBJECT

 public:
  explicit LogindSession(QObject* parent = nullptr);

  bool IsValid() const { return !path_.isEmpty(); }
  QString Id() const { return id_; }
  uint VtNumber() const { return vt_number_; }

  // Mirrors the lock state into logind's LockedHint for other tools.
  void SetLockedHint(bool locked);

 Q_SIGNALS:
  void LockRequested();
  void UnlockRequested();

 private Q_SLOTS:
  void OnLock();
  void OnUnlock();

 private:
  QString path_;
  QString id_;
  uint vt_number_ = 0;
};

}  // namespace gxde::lock

#endif  // SRC_COMMON_LOGINDSESSION_H_
