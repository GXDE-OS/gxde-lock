/*
 * Copyright (C) CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "general_setting.h"

#include <QDir>
#include <QFile>
#include <QObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

namespace {

// Reads the value the way LockStyleResolver does: QSettings keeps trailing
// "# comments" in the value, so they are cut off here.
QString ReadStyle(const QString& path) {
  const QSettings settings(path, QSettings::IniFormat);
  const QString value =
      settings.value(QStringLiteral("LockScreenStyle")).toString();
  return value.section(QLatin1Char('#'), 0, 0).trimmed();
}

class GeneralSettingTest : public QObject {
  Q_OBJECT
 private slots:
  void Persistence();
};

void GeneralSettingTest::Persistence() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString path = directory.filePath(QStringLiteral("gxde-lock.conf"));
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write(
      "# Keep this\n[General]\nOther=x11\n"
      "LockScreenStyle=modern # Keep comment\n"
      "[Theme]\nCurrent=custom\n");
  file.close();
  QVERIFY(gxde::SaveGeneralSetting(path, QStringLiteral("LockScreenStyle"),
                                   QStringLiteral("classic")));
  QCOMPARE(ReadStyle(path), QStringLiteral("classic"));
  QVERIFY(file.open(QIODevice::ReadOnly));
  const auto content = file.readAll();
  QVERIFY(content.startsWith("# Keep this\n"));
  QVERIFY(content.contains("LockScreenStyle=classic # Keep comment"));
  QVERIFY(content.contains("Other=x11"));
  QVERIFY(content.contains("[Theme]\nCurrent=custom"));
  file.close();

  const QString new_path =
      directory.filePath(QStringLiteral("new/gxde-lock.conf"));
  QVERIFY(gxde::SaveGeneralSetting(new_path, QStringLiteral("LockScreenStyle"),
                                   QStringLiteral("modern")));
  QCOMPARE(ReadStyle(new_path), QStringLiteral("modern"));
  QVERIFY(!gxde::SaveGeneralSetting(path + QStringLiteral("/invalid"),
                                    QStringLiteral("LockScreenStyle"),
                                    QStringLiteral("modern")));
}

}  // namespace

QTEST_GUILESS_MAIN(GeneralSettingTest)
#include "general_setting_test.moc"
