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

#ifndef SRC_COMMON_LOCKSTATE_H_
#define SRC_COMMON_LOCKSTATE_H_

#include <QString>

namespace gxde::lock {

// A per-session flag that exists while the locker holds the screen. The guard
// reads it after the locker died to decide whether the replacement must lock
// again at once. It lives in XDG_RUNTIME_DIR, named after the display, so it
// goes away with the user's last session.
QString LockStateFile();
void MarkSessionLocked(bool locked);
bool IsSessionMarkedLocked();

}  // namespace gxde::lock

#endif  // SRC_COMMON_LOCKSTATE_H_
