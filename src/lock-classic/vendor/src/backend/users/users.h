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

#ifndef SRC_BACKEND_USERS_USERS_H_
#define SRC_BACKEND_USERS_USERS_H_

#include <sys/types.h>

#include <string>
#include <map>

#include "src/backend/global_def/global.h"

namespace gxdm {
namespace backend {

constexpr uid_t kMinUID = 1000;
constexpr uid_t kMaxUID = 60000;

class Users {
 public:
  /**
   * @brief Just the constructor that automatically calls
   *        @c InitAccountLists()...
   * @param (none)
   * @return (void)
   */
  explicit Users();

  /**
   * @brief Get all users in the system.
   * 
   * @note System accounts and those @c /usr/sbin/nologin, or @c /bin/false
   *       accounts WILL BE OMITTED.
   * @param (none)
   * @return (void)
   */
  void InitAccountLists();

  /**
   * @brief Get all account names and display names
   * 
   * @note We're returning a map of @c {std::string,std::string}, the first
   *       field is account name and the second field is display name.
   * @param (none)
   * @return (std::map<std::string, std::string>) A map of user names.
   */
  std::map<std::string, std::string> GetUserList();

 private:
  std::map<std::string, std::string> user_list_;
};

}  // namespace backend
}  // namespace gxdm

#endif  // SRC_BACKEND_USERS_USERS_H_
