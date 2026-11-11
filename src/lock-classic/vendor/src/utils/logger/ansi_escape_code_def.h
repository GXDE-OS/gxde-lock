/*
 * Copyright (C) 2026 CharOfString <markus_verify@126.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SRC_UTILS_LOGGER_ANSI_ESCAPE_CODE_DEF_H_
#define SRC_UTILS_LOGGER_ANSI_ESCAPE_CODE_DEF_H_

#define BOLD_BLUE_FORMAT "\x1b[1;34;49m"
#define BOLD_GREEN_FORMAT "\x1b[1;32;49m"
#define BOLD_YELLOW_FORMAT "\x1b[1;33;49m"
#define BOLD_RED_FORMAT "\x1b[1;31;49m"
#define BOLD_CYAN_FORMAT "\x1b[1;36;49m"

#define BOLD_FORMAT "\x1b[1;39;49m"
#define BOLD_UNDERLINE_FORMAT "\x1b[1;4;39;49m"

#define RESET_FORMAT "\x1b[0m"

#endif  // SRC_UTILS_LOGGER_ANSI_ESCAPE_CODE_DEF_H_
