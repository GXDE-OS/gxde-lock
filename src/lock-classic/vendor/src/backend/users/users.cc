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

#include <pwd.h>
#include <sys/types.h>
#include <cstring>
#include <string>
#include <map>

#include "src/backend/users/users.h"

namespace gxdm {
namespace backend {

/**
 * @brief Just the constructor that automatically calls
 *        @c InitAccountLists()...
 * @param (none)
 * @return (void)
 */
Users::Users() {
  InitAccountLists();
}

/**
 * @brief Get all users in the system.
 * 
 * @note System accounts and those @c /usr/sbin/nologin, or @c /bin/false
 *       accounts WILL BE OMITTED.
 * @param (none)
 * @return (void)
 */
void Users::InitAccountLists() {
  setpwent();

  while (auto* cur = getpwent()) {
    // Filter out system account and nobody
    if (cur->pw_uid < kMinUID || cur->pw_uid > kMaxUID) {
      continue;
    }

    // Filter out account without shell, such as /usr/sbin/nologin, /bin/false
    std::string shell_read = cur->pw_shell ? cur->pw_shell : "false";
    if (shell_read.ends_with("nologin") || shell_read.ends_with("false")) {
      continue;
    }

    std::string display_name = cur->pw_name;
    std::string gecos_read = cur->pw_gecos ? cur->pw_gecos : "";
    std::string gecos_name = gecos_read.substr(0, gecos_read.find(','));
    if (!gecos_name.empty()) {
      display_name = gecos_name;
    }
    user_list_.insert({cur->pw_name, display_name});
  }

  endpwent();
}

/**
 * @brief Get all account names and display names
 * 
 * @note We're returning a map of @c {std::string,std::string}, the first
 *       field is account name and the second field is display name.
 * @param (none)
 * @return (std::map<std::string, std::string>) A map of user names.
 */
std::map<std::string, std::string> Users::GetUserList() {
  return user_list_;
}

}  // namespace backend
}  // namespace gxdm
