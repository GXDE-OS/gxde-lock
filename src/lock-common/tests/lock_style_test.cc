/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "lockstyle.h"
#include "lockstylemanager.h"
#include "sessionbasemodel.h"

namespace {

bool writeFile(const QString& path, const QByteArray& content) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
  return file.write(content) == content.size();
}

bool makeExecutable(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
  file.write("#!/bin/sh\n");
  file.close();
  return file.setPermissions(QFileDevice::ReadOwner |
                             QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                             QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                             QFileDevice::ReadOther | QFileDevice::ExeOther);
}

}  // namespace

class LockStyleTest : public QObject {
  Q_OBJECT

 private slots:
  void defaultsToModern();
  void defaultFallsBackToClassicWithoutNeo();
  void lockConfigWins();
  void omgFallback();
  void stripsInlineComments();
  void fallsBackWhenModernBinaryMissing();
  void managerHandsOffWhenStyleChanges();
  void managerDefersWhileVisible();
  void managerDropsRevertedChange();
  void managerIgnoresMissingBinary();
};

void LockStyleTest::defaultsToModern() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const QString bin = dir.filePath(QStringLiteral("bin"));
  QVERIFY(QDir().mkpath(bin));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-classic"))));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-neo"))));

  QVERIFY(gxde::lock::LockStyleResolver::ResolveFromFiles(
              dir.filePath(QStringLiteral("missing-gxde-lock.conf")),
              dir.filePath(QStringLiteral("missing-omg.conf")), bin) ==
          gxde::lock::LockStyle::kModern);
}

void LockStyleTest::defaultFallsBackToClassicWithoutNeo() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const QString bin = dir.filePath(QStringLiteral("bin"));
  QVERIFY(QDir().mkpath(bin));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-classic"))));

  QVERIFY(gxde::lock::LockStyleResolver::ResolveFromFiles(
              dir.filePath(QStringLiteral("missing-gxde-lock.conf")),
              dir.filePath(QStringLiteral("missing-omg.conf")), bin) ==
          gxde::lock::LockStyle::kClassic);
}

void LockStyleTest::lockConfigWins() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const QString config = dir.filePath(QStringLiteral("gxde-lock.conf"));
  const QString omg = dir.filePath(QStringLiteral("OMG.conf"));
  const QString bin = dir.filePath(QStringLiteral("bin"));
  QVERIFY(QDir().mkpath(bin));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-classic"))));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-neo"))));
  QVERIFY(writeFile(config, "[General]\nLockScreenStyle=modern\n"));
  QVERIFY(writeFile(omg, "[NextFeatReq]\nSwitchLkScrStyle=CLASSIC\n"));

  QVERIFY(gxde::lock::LockStyleResolver::ResolveFromFiles(config, omg, bin) ==
          gxde::lock::LockStyle::kModern);
}

void LockStyleTest::omgFallback() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const QString config = dir.filePath(QStringLiteral("gxde-lock.conf"));
  const QString omg = dir.filePath(QStringLiteral("OMG.conf"));
  const QString bin = dir.filePath(QStringLiteral("bin"));
  QVERIFY(QDir().mkpath(bin));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-classic"))));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-neo"))));
  QVERIFY(writeFile(config, "[General]\n"));
  QVERIFY(writeFile(omg, "[NextFeatReq]\nSwitchLkScrStyle=MODERN\n"));

  QVERIFY(gxde::lock::LockStyleResolver::ResolveFromFiles(config, omg, bin) ==
          gxde::lock::LockStyle::kModern);
}

void LockStyleTest::stripsInlineComments() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const QString config = dir.filePath(QStringLiteral("gxde-lock.conf"));
  const QString omg = dir.filePath(QStringLiteral("OMG.conf"));
  const QString bin = dir.filePath(QStringLiteral("bin"));
  QVERIFY(QDir().mkpath(bin));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-classic"))));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-neo"))));
  QVERIFY(writeFile(config,
                    "[General]\nLockScreenStyle=modern ; use the QML locker\n"));
  QVERIFY(writeFile(omg, "[NextFeatReq]\n"));

  QVERIFY(gxde::lock::LockStyleResolver::ResolveFromFiles(config, omg, bin) ==
          gxde::lock::LockStyle::kModern);
}

void LockStyleTest::fallsBackWhenModernBinaryMissing() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const QString config = dir.filePath(QStringLiteral("gxde-lock.conf"));
  const QString omg = dir.filePath(QStringLiteral("OMG.conf"));
  const QString bin = dir.filePath(QStringLiteral("bin"));
  QVERIFY(QDir().mkpath(bin));
  QVERIFY(makeExecutable(QDir(bin).filePath(
      QStringLiteral("gxde-lock-classic"))));
  QVERIFY(writeFile(config, "[General]\nLockScreenStyle=modern\n"));
  QVERIFY(writeFile(omg, "[NextFeatReq]\n"));

  QVERIFY(gxde::lock::LockStyleResolver::ResolveFromFiles(config, omg, bin) ==
          gxde::lock::LockStyle::kClassic);
}

namespace {

struct StyleFixture {
  QTemporaryDir dir;
  QString config;
  QString omg;
  QString bin;

  explicit StyleFixture(bool withNeo) {
    config = dir.filePath(QStringLiteral("gxde-lock.conf"));
    omg = dir.filePath(QStringLiteral("OMG.conf"));
    bin = dir.filePath(QStringLiteral("bin"));
    QDir().mkpath(bin);
    makeExecutable(QDir(bin).filePath(QStringLiteral("gxde-lock-classic")));
    if (withNeo) {
      makeExecutable(QDir(bin).filePath(QStringLiteral("gxde-lock-neo")));
    }
    writeFile(config, "[General]\nLockScreenStyle=classic\n");
    writeFile(omg, "[NextFeatReq]\n");
  }

  gxde::lock::LockStylePaths paths() const { return {config, omg, bin}; }
};

}  // namespace

void LockStyleTest::managerHandsOffWhenStyleChanges() {
  StyleFixture fixture(true);
  QVERIFY(fixture.dir.isValid());

  SessionBaseModel model(SessionBaseModel::LockType);
  gxde::lock::LockStyleManager manager(
      &model, gxde::lock::LockStyle::kClassic, fixture.paths());
  QSignalSpy handoff(&manager,
                     &gxde::lock::LockStyleManager::HandoffRequested);

  // Unchanged configuration: nothing to do.
  manager.Refresh();
  QCOMPARE(handoff.count(), 0);

  QVERIFY(writeFile(fixture.config, "[General]\nLockScreenStyle=modern\n"));
  manager.Refresh();
  QCOMPARE(handoff.count(), 1);
  QCOMPARE(handoff.at(0).at(0).toString(),
           QDir(fixture.bin).filePath(QStringLiteral("gxde-lock-neo")));
  QVERIFY(manager.ConfiguredStyle() == gxde::lock::LockStyle::kModern);
  QVERIFY(manager.RunningStyle() == gxde::lock::LockStyle::kClassic);
}

void LockStyleTest::managerDefersWhileVisible() {
  StyleFixture fixture(true);
  QVERIFY(fixture.dir.isValid());

  SessionBaseModel model(SessionBaseModel::LockType);
  model.setIsShow(true);

  gxde::lock::LockStyleManager manager(
      &model, gxde::lock::LockStyle::kModern, fixture.paths());
  QSignalSpy handoff(&manager,
                     &gxde::lock::LockStyleManager::HandoffRequested);

  QVERIFY(writeFile(fixture.config, "[General]\nLockScreenStyle=classic\n"));
  manager.Refresh();
  QCOMPARE(handoff.count(), 0);
  QVERIFY(manager.HandoffPending());

  model.setIsShow(false);
  QCOMPARE(handoff.count(), 1);
  QCOMPARE(handoff.at(0).at(0).toString(),
           QDir(fixture.bin).filePath(QStringLiteral("gxde-lock-classic")));
  QVERIFY(!manager.HandoffPending());
}

void LockStyleTest::managerDropsRevertedChange() {
  StyleFixture fixture(true);
  QVERIFY(fixture.dir.isValid());

  SessionBaseModel model(SessionBaseModel::LockType);
  model.setIsShow(true);

  gxde::lock::LockStyleManager manager(
      &model, gxde::lock::LockStyle::kClassic, fixture.paths());
  QSignalSpy handoff(&manager,
                     &gxde::lock::LockStyleManager::HandoffRequested);

  QVERIFY(writeFile(fixture.config, "[General]\nLockScreenStyle=modern\n"));
  manager.Refresh();
  QVERIFY(manager.HandoffPending());

  // Switched back before unlocking: the deferred handoff must not fire.
  QVERIFY(writeFile(fixture.config, "[General]\nLockScreenStyle=classic\n"));
  model.setIsShow(false);
  QCOMPARE(handoff.count(), 0);
  QVERIFY(!manager.HandoffPending());
}

void LockStyleTest::managerIgnoresMissingBinary() {
  StyleFixture fixture(false);
  QVERIFY(fixture.dir.isValid());

  SessionBaseModel model(SessionBaseModel::LockType);
  gxde::lock::LockStyleManager manager(
      &model, gxde::lock::LockStyle::kClassic, fixture.paths());
  QSignalSpy handoff(&manager,
                     &gxde::lock::LockStyleManager::HandoffRequested);

  QVERIFY(writeFile(fixture.config, "[General]\nLockScreenStyle=modern\n"));
  manager.Refresh();
  QCOMPARE(handoff.count(), 0);
  QVERIFY(manager.ConfiguredStyle() == gxde::lock::LockStyle::kClassic);
}

QTEST_GUILESS_MAIN(LockStyleTest)
#include "lock_style_test.moc"
