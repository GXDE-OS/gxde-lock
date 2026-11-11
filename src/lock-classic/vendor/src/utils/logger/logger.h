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

#ifndef SRC_UTILS_LOGGER_LOGGER_H_
#define SRC_UTILS_LOGGER_LOGGER_H_

namespace gxdm {
namespace utils {

/**
 * @brief Log level used to control output style and destination stream.
 */
enum class LogLevel : int {
  kInfo = 0,
  kOk = 1,
  kWarning = 2,
  kError = 3,
  kDebug = 4
};

class Logger {
 public:
  /**
   * @brief Print the log content to stdout/stderr.
   * 
   * This function outputs the log message with the approate header. e.g.
   * [INFO] Hello.
   * 
   * If the log level is @c kError, the message will be printed to @c stderr.
   * Otherwise the log is printed through @c stdout.
   * 
   * @param level (LogLevel) The log level.
   * @param content (const char*) The log message.
   * @param ... (variadic) Optional format arguments.
   * @return void
   */
  static void WriteLog(LogLevel level, const char* content, ...);

  /**
   * @brief Print the log title and content to stdout/stderr.
   * 
   * This function outputs the log title and message with the approate header.
   * e.g. [INFO] Greetings: Hello.
   * 
   * In the example above, "Greetings" is the title and "Hello." is the message
   * body.
   * 
   * If the log level is @c kError, the message will be printed to @c stderr.
   * Otherwise the log is printed through @c stdout.
   * 
   * @param level (LogLevel) The log level.
   * @param title (const char*) The log title.
   * @param content (const char*) The log message.
   * @param ... (variadic) Optional format arguments.
   * @return void
   */
  static void WriteLogWithTitle(LogLevel level, const char* title,
    const char* content, ...);

  /**
   * @brief Print the log tag, title and content to stdout/stderr.
   * 
   * This function outputs the log tag, title and message with the approate
   * header. e.g. [INFO] Main - Greetings: Hello.
   * 
   * In the example above, "Main" is the tag, "Greetings" is the title and
   * "Hello." is the message body.
   * 
   * If the log level is @c kError, the message will be printed to @c stderr.
   * Otherwise the log is printed through @c stdout.
   * 
   * @param level (LogLevel) The log level.
   * @param tag (const char*) The log tag.
   * @param title (const char*) The log title.
   * @param content (const char*) The log message.
   * @param ... (variadic) Optional format arguments.
   * @return void
   */
  static void WriteLogWithTag(LogLevel level, const char* tag,
    const char* title, const char* content, ...);

 private:
  /**
   * @brief Print the colored log level to stdout/stderr.
   * 
   * This is an internal function. It prints the log header (e.g. @c [INFO]) to
   * @c stdout (or @c stderr, if the log level is @c kError.) These headers
   * are colored using ANSI escape codes
   * 
   * @see src/utils/log/ansi_escape_code_def.h for the color definitions.
   *
   * @param level (LogLevel) The log level.
   * @return void
   */
  static void PrintLevelPrefix(LogLevel level);
};

}  // namespace utils
}  // namespace gxdm

#endif  // SRC_UTILS_LOGGER_LOGGER_H_
