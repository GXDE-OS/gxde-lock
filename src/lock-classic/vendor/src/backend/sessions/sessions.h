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

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifndef SRC_BACKEND_SESSIONS_SESSIONS_H_
#define SRC_BACKEND_SESSIONS_SESSIONS_H_

namespace gxdm {
namespace backend {

enum class SessionType {
  kX11 = 0,
  kWayland = 1
};

enum class ProfileReadStatus {
  kNormalExit = 0,
  kHidden = 1,
  kError = 2
};

struct SessionProfile {
  std::string key = "CHANGEME";
  std::string session_name = "UNKNOWN";
  SessionType session_type = SessionType::kX11;
  std::string cmd = "echo \"Unknown command.\"";
  std::string xdg_session_desktop = "UNKNOWN";
  std::string xdg_current_desktop = "UNKNOWN";
};

constexpr const std::string_view kX11SessionDir = "/usr/share/xsessions";
constexpr const std::string_view kWaylandSessionDir =
"/usr/share/wayland-sessions";

class Sessions {
 public:
  /**
   * @brief Just constructor...
   * 
   * @details Calls @c DiscoverSessions() to initialize internal fields.
   * @param omit_warning (bool) Prints a warning if the session file is
   *                            omitted. defaulting to false.
   * @return (void)
  */
  explicit Sessions(bool omit_warning = false);

  /**
   * @brief Discover installed sessions.
   * 
   * @details It iterates through all the .desktop file in x11
   *          sessions/wayland sessions directory and try to get session
   *          information to fill in the @c SessionProfile. If it failed
   *          to fill in all fields in the @c SessionProfile, it will omit
   *          that session.
   * @param omit_warning (bool) Prints a warning if the session file is
   *                            omitted. defaulting to false.
   * @return (void)
   */
  void DiscoverSessions(bool omit_warning = false);

  /**
   * @brief Get session names list.
   * 
   * @note Each name could be a key for the session hash table.
   * @param (none)
   * @return (std::vector<std::string>) A vector of session names.
   */
  std::vector<std::string> GetSessionNameList();

  /**
   * @brief Get session hash table.
   * 
   * @note The key is session name, the value is a @c SessionProfile.
   * @param (none)
   * @return (std::unordered_map<std::string, SessionProfile>) An unordered map
   *                                                           of session
   *                                                           information.
   */
  std::unordered_map<std::string, SessionProfile> GetSessionHashTable();

 private:
  std::unordered_map<std::string, SessionProfile> session_hash_table_;
  std::vector<std::string> session_names_;
};

}  // namespace backend
}  // namespace gxdm

#endif  // SRC_BACKEND_SESSIONS_SESSIONS_H_
