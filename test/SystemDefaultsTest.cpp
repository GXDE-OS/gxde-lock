#include "SystemDefaults.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class SystemDefaultsTest : public QObject
{
    Q_OBJECT

private slots:
    void selectsDeepinOnGxde();
    void matchesGxdeCaseInsensitively();
    void keepsRememberedSessionOnGxde();
    void leavesOtherSystemsUnspecified();

private:
    QString writeOsRelease(const QByteArray &contents);

    QTemporaryDir m_temporaryDir;
};

QString SystemDefaultsTest::writeOsRelease(const QByteArray &contents)
{
    const QString path = m_temporaryDir.filePath(QStringLiteral("os-release"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return QString();
    file.write(contents);
    file.close();
    return path;
}

void SystemDefaultsTest::selectsDeepinOnGxde()
{
    const QString path = writeOsRelease("NAME=GXDE\nID=GXDE\n");
    QCOMPARE(LockSystemDefaults::operatingSystemId(path), QStringLiteral("GXDE"));
    QVERIFY(LockSystemDefaults::isGxdeOperatingSystem(path));
    QCOMPARE(LockSystemDefaults::defaultSessionDesktopFile(path),
        QStringLiteral("deepin.desktop"));
}

void SystemDefaultsTest::matchesGxdeCaseInsensitively()
{
    const QString path = writeOsRelease("ID=gXdE\n");
    QVERIFY(LockSystemDefaults::isGxdeOperatingSystem(path));
    QCOMPARE(LockSystemDefaults::defaultSessionDesktopFile(path),
        QStringLiteral("deepin.desktop"));
}

void SystemDefaultsTest::keepsRememberedSessionOnGxde()
{
    const QString path = writeOsRelease("ID=GXDE\n");
    QCOMPARE(LockSystemDefaults::initialSessionDesktopFile(
                 QStringLiteral("gxde-wlcom.desktop"), path),
        QStringLiteral("gxde-wlcom.desktop"));
}

void SystemDefaultsTest::leavesOtherSystemsUnspecified()
{
    const QString path = writeOsRelease("NAME=Debian GNU/Linux\nID=debian\n");
    QVERIFY(!LockSystemDefaults::isGxdeOperatingSystem(path));
    QCOMPARE(LockSystemDefaults::defaultSessionDesktopFile(path), QString());
}

QTEST_MAIN(SystemDefaultsTest)
#include "SystemDefaultsTest.moc"
