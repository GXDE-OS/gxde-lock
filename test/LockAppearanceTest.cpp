#include <QFile>
#include <QImage>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "LockAppearance.h"

class LockAppearanceTest : public QObject {
  Q_OBJECT

 private slots:
  void readsGlobalAppearance();
  void exposesDefaultWallpaperPaths();
  void detectsUnconfiguredWallpaper();
  void acceptsOnlyReadableImages();
};

void LockAppearanceTest::readsGlobalAppearance() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString wallpaperPath = directory.filePath(QStringLiteral("wallpaper"));
  QFile wallpaper(wallpaperPath);
  QVERIFY(wallpaper.open(QIODevice::WriteOnly));
  QCOMPARE(wallpaper.write("image"), 5);
  wallpaper.close();

  const uint uid = 1000;
  const QString lockOverridePath = directory.filePath(
    QStringLiteral("lock-wallpaper-override"));
  QFile lockOverride(lockOverridePath);
  QVERIFY(lockOverride.open(QIODevice::WriteOnly));
  QCOMPARE(lockOverride.write("image"), 5);
  lockOverride.close();

  const QString statePath = directory.filePath(QStringLiteral("state.conf"));
  QSettings settings(statePath, QSettings::IniFormat);
  settings.setValue(
    QStringLiteral("Lock/CursorTheme"), QStringLiteral("gxde"));
  settings.setValue(QStringLiteral("Lock/Wallpaper"), wallpaperPath);
  settings.sync();

  QCOMPARE(
    LockAppearance::cursorTheme(statePath), QStringLiteral("gxde"));
  QCOMPARE(LockAppearance::wallpaper(statePath), wallpaperPath);
  QVERIFY(LockAppearance::hasConfiguredWallpaper(statePath));
  QCOMPARE(LockAppearance::lockWallpaperOverride(
    uid, directory.path()), lockOverridePath);
  QCOMPARE(LockAppearance::lockWallpaperOverridePath(
    uid, directory.path()), lockOverridePath);
  QVERIFY(QFile::remove(lockOverridePath));
  QCOMPARE(LockAppearance::lockWallpaperOverride(
    uid, directory.path()), QString());
}

void LockAppearanceTest::exposesDefaultWallpaperPaths() {
  QCOMPARE(LockAppearance::gxdeDefaultWallpaper(),
    QStringLiteral("/usr/share/backgrounds/default_background.jpg"));
  QCOMPARE(LockAppearance::ddeLockDefaultWallpaper(),
    QStringLiteral(":/theme/background/default_background.jpg"));
}

void LockAppearanceTest::detectsUnconfiguredWallpaper() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  // state.conf 存在但没有写入 [Lock] Wallpaper
  const QString statePath = directory.filePath(QStringLiteral("state.conf"));
  QSettings settings(statePath, QSettings::IniFormat);
  settings.setValue(
    QStringLiteral("Lock/CursorTheme"), QStringLiteral("gxde"));
  settings.sync();

  QVERIFY(!LockAppearance::hasConfiguredWallpaper(statePath));
  // state.conf 文件不存在时同样视为未配置
  QVERIFY(!LockAppearance::hasConfiguredWallpaper(
    directory.filePath(QStringLiteral("missing.conf"))));

  // ClearWallpaper 写入空值 "Wallpaper="，同样视为未配置
  settings.setValue(QStringLiteral("Lock/Wallpaper"), QString());
  settings.sync();
  QVERIFY(!LockAppearance::hasConfiguredWallpaper(statePath));
}

void LockAppearanceTest::acceptsOnlyReadableImages() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString imagePath = directory.filePath(QStringLiteral("image.png"));
  QImage image(4, 4, QImage::Format_RGB32);
  image.fill(Qt::red);
  QVERIFY(image.save(imagePath));
  QCOMPARE(LockAppearance::readableImage(imagePath), imagePath);
  QCOMPARE(LockAppearance::readableImage(
    QUrl::fromLocalFile(imagePath).toString()), imagePath);

  const QString textPath = directory.filePath(QStringLiteral("text.png"));
  QFile text(textPath);
  QVERIFY(text.open(QIODevice::WriteOnly));
  QCOMPARE(text.write("image"), 5);
  text.close();
  QCOMPARE(LockAppearance::readableImage(textPath), QString());

  QCOMPARE(LockAppearance::readableImage(
    directory.filePath(QStringLiteral("missing.png"))), QString());
  QCOMPARE(LockAppearance::readableImage(directory.path()), QString());
  QCOMPARE(LockAppearance::readableImage(QString()), QString());
}

QTEST_GUILESS_MAIN(LockAppearanceTest)
#include "LockAppearanceTest.moc"
