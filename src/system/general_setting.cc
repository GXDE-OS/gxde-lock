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

#include "general_setting.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringList>

namespace gxde {
namespace {

QString WithoutComment(const QString& line) {
  const int comment_position = line.indexOf(QLatin1Char('#'));
  return (comment_position < 0 ? line : line.left(comment_position)).trimmed();
}

QString SectionName(const QString& line) {
  const QString config_line = WithoutComment(line);
  if (!config_line.startsWith(QLatin1Char('[')) ||
      !config_line.endsWith(QLatin1Char(']'))) {
    return QString();
  }
  return config_line.mid(1, config_line.size() - 2).trimmed();
}

QString AssignmentName(const QString& line) {
  const QString config_line = WithoutComment(line);
  const int separator = config_line.indexOf(QLatin1Char('='));
  if (separator < 0) return QString();
  return config_line.left(separator).trimmed();
}

QString SettingLine(const QString& original_line, const QString& key,
                    const QString& value) {
  const int comment_position = original_line.indexOf(QLatin1Char('#'));
  const QString comment = comment_position < 0
                              ? QString()
                              : original_line.mid(comment_position).trimmed();
  return QStringLiteral("%1=%2%3").arg(
      key, value,
      comment.isEmpty() ? QString() : QStringLiteral(" ") + comment);
}

}  // namespace

bool SaveGeneralSetting(const QString& config_path, const QString& key,
                        const QString& value) {
  const QFileInfo config_info(config_path);
  if (!QDir().mkpath(config_info.absolutePath())) {
    return false;
  }

  QStringList lines;
  QFile current_config(config_path);
  if (current_config.exists()) {
    if (!current_config.open(QIODevice::ReadOnly)) {
      return false;
    }

    const QString content = QString::fromUtf8(current_config.readAll());
    lines = content.split(QLatin1Char('\n'));
    if (content.endsWith(QLatin1Char('\n'))) lines.removeLast();
  }

  bool in_general = true;
  bool saw_general = false;
  bool wrote_setting = false;
  int first_non_general_section = -1;
  int general_insert_index = lines.size();

  for (int i = 0; i < lines.size(); ++i) {
    const QString section = SectionName(lines.at(i));
    if (!section.isEmpty()) {
      if (section == QLatin1String("General")) {
        in_general = true;
        saw_general = true;
        general_insert_index = i + 1;
      } else {
        if (in_general && first_non_general_section < 0)
          first_non_general_section = i;
        if (in_general) general_insert_index = i;
        in_general = false;
      }
      continue;
    }

    if (!in_general) continue;

    if (AssignmentName(lines.at(i)) == key) {
      lines[i] = SettingLine(lines.at(i), key, value);
      wrote_setting = true;
    } else {
      general_insert_index = i + 1;
    }
  }

  if (!wrote_setting) {
    const QString config_line = SettingLine(QString(), key, value);
    if (lines.isEmpty()) {
      lines << QStringLiteral("[%1]").arg(QStringLiteral("General"))
            << config_line;
    } else if (saw_general || first_non_general_section >= 0) {
      const int insert_index =
          saw_general ? general_insert_index : first_non_general_section;
      lines.insert(insert_index, config_line);
    } else {
      lines << config_line;
    }
  }

  QSaveFile saved_config(config_path);
  if (!saved_config.open(QIODevice::WriteOnly)) return false;

  const QByteArray content =
      (lines.join(QLatin1Char('\n')) + QLatin1Char('\n')).toUtf8();
  if (saved_config.write(content) != content.size()) return false;

  if (!saved_config.commit()) return false;

  QFile::setPermissions(config_path,
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                            QFileDevice::ReadGroup | QFileDevice::ReadOther);
  return true;
}

}  // namespace gxde
