/***************************************************************************
 * Copyright (c) 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 ***************************************************************************/

#ifndef SRC_SYSTEM_GENERAL_SETTING_H_
#define SRC_SYSTEM_GENERAL_SETTING_H_

#include <QString>

namespace gxde {

// Sets `key` in the [General] section of the INI file at `config_path`,
// keeping every other line, section and comment. The file and its directory
// are created when missing; the write is atomic.
bool SaveGeneralSetting(const QString& config_path, const QString& key,
                        const QString& value);

}  // namespace gxde

#endif  // SRC_SYSTEM_GENERAL_SETTING_H_
