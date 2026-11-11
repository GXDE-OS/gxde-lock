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

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

#include "src/backend/sessions/sessions.h"
#include "src/backend/global_def/global.h"
#include "src/utils/logger/logger.h"

namespace gxdm {
namespace backend {
namespace {

/**
 * @brief Strip leading and trailing whitespace in the given string.
 *
 * @param value (const std::string&) The target string.
 * @return (std::string) The cleaned copy.
 */
std::string StripString(const std::string& value) {
  std::size_t content_begin = 0;
  std::size_t content_end = value.size();

  while (std::isspace(static_cast<unsigned char>(value[content_begin]))) {
    if (content_begin >= content_end) {
      break;
    } else {
      content_begin += 1;
    }
  }

  while (std::isspace(static_cast<unsigned char>(value[content_end - 1]))) {
    if (content_end <= content_begin) {
      break;
    } else {
      content_end -= 1;
    }
  }

  std::string result = value.substr(content_begin, content_end -
    content_begin);
  return result;
}

/**
 * @brief The to lower operation to the string.
 * 
 * @details Iterates through the string and call tolower() for each character.
 * @param string_read (std::string&) string input
 * @return (std::string) The transformed string.
 */
std::string ToLower(const std::string& string_read) {
  std::string result = string_read;
  for (char& c : result) {
    unsigned char cur = static_cast<unsigned char>(c);
    if (cur <= 127) {
      // If current character is an ACSII character then it is possible to call
      // tolower()
      c = std::tolower(cur);
    }
  }

  return result;
}

/**
 * @brief Parse a single .desktop session file and returns a @c SessionProfile.
 *
 * @details We manually parse each line of the .desktop file.
 * @param path    (const std::filesystem::path&) Path to the file.
 * @param type    (SessionType)                  The session type.
 * @param profile (SessionProfile*)              Where to write the output.
 * @param logging (bool)                         Whether to print log.
 * @return (ProfileReadStatus) Reader status.
 */
ProfileReadStatus ReadSessionFile(const std::filesystem::path& path,
    SessionType type, SessionProfile* profile, bool logging) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return ProfileReadStatus::kError;
  }

  bool is_desktop_entry = false;
  bool has_cmd = false;
  bool has_desktop_names = false;

  int unknown_count = 1;
  std::string name_gen = "Unknown Session";

  std::string cur_line_raw;
  while (std::getline(file, cur_line_raw)) {
    std::string cur_line = StripString(cur_line_raw);

    // Skip empty lines and comments
    if (cur_line.empty() || cur_line.front() == '#') {
      continue;
    }

    // Skip any sections that is NOT "Desktop Entry"
    // Here is how it works: If we hit a section that is not called "Desktop
    // Entry", we mark is_desktop_entry as false. Then for next lines if
    // is_desktop_entry is false we ignore it. When the section is "Desktop
    // Entry" then is_desktop_entry will be marked as true and we will resume
    // reading next lines.
    if (cur_line.front() == '[') {
      is_desktop_entry = (cur_line == "[Desktop Entry]");
      continue;
    }

    if (!is_desktop_entry) {
      continue;
    }

    // If there is no = in the single line then it is NOT a key/value thing.
    std::size_t separator_search = cur_line.find('=');
    if (separator_search == std::string::npos) {
      continue;
    }

    // Get key & value
    std::string key = StripString(cur_line.substr(0, separator_search));
    std::string value_raw = StripString(cur_line.substr(separator_search + 1));

    if (ToLower(key) == "name") {
      if (value_raw.empty()) {
        if (unknown_count > 1) {
          name_gen += " <";
          name_gen += std::to_string(unknown_count);
          name_gen += ">";
        }

        if (logging) {
          utils::Logger::WriteLogWithTag(utils::LogLevel::kWarning,
            BACKEND_TAG, "Failed to get a session",
            "It was named as Unknown Session.");
        }
      } else {
        profile->session_name = value_raw;
      }
    } else if (ToLower(key) == "exec") {
      if (value_raw.empty()) {
        has_cmd = false;
        if (logging) {
          utils::Logger::WriteLogWithTag(utils::LogLevel::kError,
            BACKEND_TAG, "Failed to get session exec",
            "The \"Exec\" key is missing.");
        }
      } else {
        has_cmd = true;
        profile->cmd = value_raw;
      }
    } else if (ToLower(key) == "desktopnames") {
      if (value_raw.empty()) {
        has_desktop_names = false;
        if (logging) {
          utils::Logger::WriteLogWithTag(utils::LogLevel::kError,
            BACKEND_TAG, "Failed to get session desktop names",
            "The \"DesktopNames\" key is missing.");
        }
      } else {
        has_desktop_names = true;
        profile->xdg_current_desktop = value_raw;
      }
    } else if (key == "Hidden" || key == "NoDisplay") {
      if (ToLower(value_raw) == "true") {
        // Skip hidden session
        return ProfileReadStatus::kHidden;
      }
    }
  }

  profile->session_type = type;
  profile->xdg_session_desktop = path.stem().string();

  // "DesktopNames" is optional...
  if (!has_desktop_names) {
    profile->xdg_current_desktop = path.stem().string();
  }

  std::string key_gen;
  switch (type) {
    case SessionType::kX11:
      key_gen.append("X11-");
      break;
    case SessionType::kWayland:
      key_gen.append("WAYLAND-");
      break;
  }

  key_gen.append(profile->session_name);
  if (has_desktop_names) {
    key_gen.append("-");
    key_gen.append(profile->xdg_current_desktop);
  }
  profile->key = key_gen;

  if (has_cmd) {
    return ProfileReadStatus::kNormalExit;
  } else {
    return ProfileReadStatus::kError;
  }
}

/**
 * @brief Get the session dir.
 * 
 * @param value (SessionType) The session type.
 * @return (std::string_view) The session dir of the type choosed.
 */
std::string_view GetSessionDir(SessionType value) {
  switch (value) {
    case SessionType::kX11:
      return kX11SessionDir;

    case SessionType::kWayland:
      return kWaylandSessionDir;
  }
}

}  // namespace

/**
 * @brief Just constructor...
 * 
 * @details Calls @c DiscoverSessions() to initialize internal fields.
 * @param omit_warning (bool) Prints a warning if the session file is
 *                            omitted. defaulting to false.
 * @return (void)
 */
Sessions::Sessions(bool omit_warning) {
  DiscoverSessions(omit_warning);
}

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
void Sessions::DiscoverSessions(bool omit_warning) {
  session_hash_table_.clear();
  session_names_.clear();

  const std::vector<SessionType> kSessionTypes = {
    SessionType::kX11,
    SessionType::kWayland
  };

  for (const auto& cur : kSessionTypes) {
    std::string_view session_directory = GetSessionDir(cur);

    std::error_code err_code;
    if (!std::filesystem::is_directory(session_directory, err_code)) {
      continue;
    }

    for (const auto& cur_session : std::filesystem::directory_iterator(
        session_directory, err_code)) {
      if (err_code) {
        if (omit_warning) {
          utils::Logger::WriteLogWithTag(utils::LogLevel::kError, BACKEND_TAG,
            "Failed to get session dir",
            "Error happened while getting session directory, omitting...");
        }
        break;
      }

      if (!cur_session.is_regular_file() || cur_session.path().extension() !=
        ".desktop") {
        continue;
      }

      SessionProfile profile_gen;
      ProfileReadStatus read_status = ReadSessionFile(cur_session.path(), cur,
        &profile_gen, omit_warning);
      if (read_status == ProfileReadStatus::kError) {
        if (omit_warning) {
          utils::Logger::WriteLogWithTag(utils::LogLevel::kWarning, BACKEND_TAG,
            "Omitting Session", "Omitting 「%s」 due to error occurred.",
            profile_gen.session_name.c_str());
        }
        continue;
      }

      if (read_status == ProfileReadStatus::kHidden) {
        if (omit_warning) {
          utils::Logger::WriteLogWithTag(utils::LogLevel::kWarning, BACKEND_TAG,
            "Omitting Session", "Omitting 「%s」 because it is hidden.",
            profile_gen.session_name.c_str());
        }
        continue;
      }

      if (session_hash_table_.count(profile_gen.key) > 0) {
        if (omit_warning) {
          utils::Logger::WriteLogWithTag(utils::LogLevel::kWarning, BACKEND_TAG,
            "Omitting Session", "Omitting 「%s」 due to duplication.",
            profile_gen.session_name.c_str());
        }
      }

      session_names_.push_back(profile_gen.key);
      session_hash_table_.emplace(profile_gen.key,
        std::move(profile_gen));
    }
  }
}

/**
 * @brief Get session names list.
 * 
 * @note Each name could be a key for the session hash table.
 * @param (none)
 * @return (std::vector<std::string>) A vector of session names.
 */
std::vector<std::string> Sessions::GetSessionNameList() {
  return session_names_;
}

/**
 * @brief Get session hash table.
 * 
 * @note The key is session name, the value is a @c SessionProfile.
 * @param (none)
 * @return (std::unordered_map<std::string, SessionProfile>) An unordered map
 *                                                           of session
 *                                                           information.
 */
std::unordered_map<std::string, SessionProfile>
Sessions::GetSessionHashTable() {
  return session_hash_table_;
}

}  // namespace backend
}  // namespace gxdm
