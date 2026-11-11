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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "src/utils/logger/logger.h"
#include "src/utils/logger/ansi_escape_code_def.h"

namespace gxdm {
namespace utils {

/**
 * @brief Print the colored log level to stdout/stderr.
 * 
 * This is an internal function. It prints the log header (e.g. @c [INFO]) to
 * @c stdout (or @c stderr, if the log level is @c LogLevel::kError.) These headers
 * are colored using ANSI escape codes
 * 
 * @see src/utils/log/ansi_escape_code_def.h for the color definitions.
 *
 * @param level (LogLevel) The log level.
 * @return void
 */
void Logger::PrintLevelPrefix(LogLevel level) {
  switch (level) {
    case LogLevel::kInfo:
      fprintf(stdout, "%s", BOLD_BLUE_FORMAT);
      fprintf(stdout, "[INFO]");
      fprintf(stdout, "%s ", RESET_FORMAT);
      break;

    case LogLevel::kOk:
      fprintf(stdout, "%s", BOLD_GREEN_FORMAT);
      fprintf(stdout, "[ OK ]");
      fprintf(stdout, "%s ", RESET_FORMAT);
      break;

    case LogLevel::kWarning:
      fprintf(stdout, "%s", BOLD_YELLOW_FORMAT);
      fprintf(stdout, "[WARNING]");
      fprintf(stdout, "%s ", RESET_FORMAT);
      break;

    case LogLevel::kError:
      fprintf(stderr, "%s", BOLD_RED_FORMAT);
      fprintf(stderr, "[ERROR]");
      fprintf(stderr, "%s ", RESET_FORMAT);
      break;

    case LogLevel::kDebug:
      fprintf(stdout, "%s", BOLD_CYAN_FORMAT);
      fprintf(stdout, "[DEBUG] ");
      fprintf(stdout, "%s ", RESET_FORMAT);
      break;

    default:
      fprintf(stdout, "%s", BOLD_CYAN_FORMAT);
      fprintf(stdout, "[DEBUG] ");
      fprintf(stdout, "%s ", RESET_FORMAT);
      break;
  }
}

/**
 * @brief Print the log content to stdout/stderr.
 * 
 * This function outputs the log message with the approate header. e.g.
 * [INFO] Hello.
 * 
 * If the log level is @c LogLevel::kError, the message will be printed to @c stderr.
 * Otherwise the log is printed through @c stdout.
 * 
 * @param level (LogLevel) The log level.
 * @param content (const char*) The log message.
 * @param ... (variadic) Optional format arguments.
 * @return void
 */
void Logger::WriteLog(LogLevel level, const char* content, ...) {
  PrintLevelPrefix(level);

  va_list args;
  va_start(args, content);
  vfprintf(level == LogLevel::kError ? stderr : stdout, content, args);
  fprintf(level == LogLevel::kError ? stderr : stdout, "\n");
  va_end(args);
}

/**
 * @brief Print the log title and content to stdout/stderr.
 * 
 * This function outputs the log title and message with the approate header.
 * e.g. [INFO] Greetings: Hello.
 * 
 * In the example above, "Greetings" is the title and "Hello." is the message
 * body.
 * 
 * If the log level is @c LogLevel::kError, the message will be printed to @c stderr.
 * Otherwise the log is printed through @c stdout.
 * 
 * @param level (LogLevel) The log level.
 * @param title (const char*) The log title.
 * @param content (const char*) The log message.
 * @param ... (variadic) Optional format arguments.
 * @return void
 */
void Logger::WriteLogWithTitle(LogLevel level, const char* title,
    const char* content, ...) {
  PrintLevelPrefix(level);

  fprintf(level == LogLevel::kError ? stderr : stdout, "%s", BOLD_FORMAT);
  fprintf(level == LogLevel::kError ? stderr : stdout, "%s", title);
  fprintf(level == LogLevel::kError ? stderr : stdout, "%s", RESET_FORMAT);
  fprintf(level == LogLevel::kError ? stderr : stdout, "%s", ": ");

  va_list args;
  va_start(args, content);
  vfprintf(level == LogLevel::kError ? stderr : stdout, content, args);
  fprintf(level == LogLevel::kError ? stderr : stdout, "\n");
  va_end(args);
}

/**
 * @brief Print the log tag, title and content to stdout/stderr.
 * 
 * This function outputs the log tag, title and message with the approate
 * header. e.g. [INFO] Main - Greetings: Hello.
 * 
 * In the example above, "Main" is the tag, "Greetings" is the title and
 * "Hello." is the message body.
 * 
 * If the log level is @c LogLevel::kError, the message will be printed to @c stderr.
 * Otherwise the log is printed through @c stdout.
 * 
 * @param level (LogLevel) The log level.
 * @param tag (const char*) The log tag.
 * @param title (const char*) The log title.
 * @param content (const char*) The log message.
 * @param ... (variadic) Optional format arguments.
 * @return void
 */
void Logger::WriteLogWithTag(LogLevel level, const char* tag, const char* title,
    const char* content, ...) {
  PrintLevelPrefix(level);

  fprintf(level == LogLevel::kError ? stderr : stdout, "%s",
    BOLD_UNDERLINE_FORMAT);
  fprintf(level == LogLevel::kError ? stderr : stdout, "%s", tag);
  fprintf(level == LogLevel::kError ? stderr : stdout, "%s", " - ");
  fprintf(level == LogLevel::kError ? stderr : stdout, "%s", title);
  fprintf(level == LogLevel::kError ? stderr : stdout, "%s", RESET_FORMAT);
  fprintf(level == LogLevel::kError ? stderr : stdout, "%s", ": ");

  va_list args;
  va_start(args, content);
  vfprintf(level == LogLevel::kError ? stderr : stdout, content, args);
  fprintf(level == LogLevel::kError ? stderr : stdout, "\n");
  va_end(args);
}

}  // namespace utils
}  // namespace gxdm
