#include "userinfo.h"

#include "LockAppearance.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QFileInfo>
#include <QStringList>
#include <QTimer>
#include <QGSettings/QGSettings>

#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#define LOCK_AUTH_NUM 5

static bool checkUserIsNoPWGrp(User const * user)
{
    if (user->type() == User::ADDomain) {
        return false;
    }

    // Caution: 32 here is unreliable, and there may be more
    // than this number of groups that the user joins.

    int ngroups = 32;
    gid_t groups[32];
    struct passwd pw;
    struct group gr;

    /* Fetch passwd structure (contains first group ID for user) */

    pw = *getpwnam(user->name().toUtf8().data());

    /* Retrieve group list */

    if (getgrouplist(user->name().toUtf8().data(), pw.pw_gid, groups, &ngroups) == -1) {
        fprintf(stderr, "getgrouplist() returned -1; ngroups = %d\n",
                ngroups);
        return false;
    }

    /* Display list of retrieved groups, along with group names */

    for (int i = 0; i < ngroups; i++) {
        gr = *getgrgid(groups[i]);
        if (QString(gr.gr_name) == QString("nopasswdlogin")) {
            return true;
        }
    }

    return false;
}

static QString deepinAccountsUserPath(const QString& userName) {
    QDBusInterface accounts(QStringLiteral("com.deepin.daemon.Accounts"),
        QStringLiteral("/com/deepin/daemon/Accounts"),
        QStringLiteral("com.deepin.daemon.Accounts"),
        QDBusConnection::systemBus());

    if (!accounts.isValid()) {
        return QString();
    }

    const QDBusReply<QString> userPath = accounts.call(
        QStringLiteral("FindUserByName"), userName);
    return userPath.isValid() ? userPath.value() : QString();
}

static QString deepinAccountsAvatar(const QString& userName) {
    const QString userPath = deepinAccountsUserPath(userName);
    if (userPath.isEmpty()) {
        return QString();
    }

    QDBusInterface user(QStringLiteral("com.deepin.daemon.Accounts"),
        userPath,
        QStringLiteral("com.deepin.daemon.Accounts.User"),
        QDBusConnection::systemBus());

    return user.isValid() ? LockAppearance::readableImage(
        user.property("IconFile").toString()) : QString();
}

static QString gsettingsLockBackground() {
    const QByteArray schema("com.deepin.dde.appearance");
    if (!QGSettings::isSchemaInstalled(schema)) {
        return QString();
    }

    QGSettings settings(schema);
    const QStringList keys = settings.keys();
    QString key;
    if (keys.contains(QStringLiteral("backgroundUris"))) {
        key = QStringLiteral("backgroundUris");
    } else if (keys.contains(QStringLiteral("background-uris"))) {
        key = QStringLiteral("background-uris");
    } else {
        return QString();
    }

    const QStringList backgrounds = settings.get(key).toStringList();
    for (const QString& background : backgrounds) {
        const QString path = LockAppearance::readableImage(background);
        if (!path.isEmpty()) {
            return path;
        }
    }
    return QString();
}

static QString lockBackgroundForUser(const QString& userName, uid_t uid) {
    // 1-3. The gxde-lock override, Deepin Accounts and AccountsService.
    const QString userBackground =
        LockAppearance::userLockWallpaper(userName, uid);
    if (!userBackground.isEmpty())
        return userBackground;

    // 4. GSettings com.deepin.dde.appearance. Only meaningful when this
    //    process runs as the user itself (e.g. the in-session locker), where
    //    the user's real desktop/lock wallpaper can be read.
    if (uid == getuid()) {
        const QString gsettingsBackground = gsettingsLockBackground();
        if (!gsettingsBackground.isEmpty()) {
            qInfo() << "(Frontend) Lock background: Using GSettings for"
                << userName << gsettingsBackground;
            return gsettingsBackground;
        }
    }

    // 5. Global greeter wallpaper (state.conf) and finally the built-in one.
    qWarning() << "(Frontend) Lock background: No usable user wallpaper for"
        << userName << "; using the greeter wallpaper";
    return LockAppearance::wallpaper();
}

static QString accountsServiceAvatar(const QString& userName) {
    QDBusInterface accounts(QStringLiteral("org.freedesktop.Accounts"),
        QStringLiteral("/org/freedesktop/Accounts"),
        QStringLiteral("org.freedesktop.Accounts"),
        QDBusConnection::systemBus());

    if (!accounts.isValid()) {
        return QString();
    }

    const QDBusReply<QDBusObjectPath> userPath = accounts.call(
        QStringLiteral("FindUserByName"), userName);

    if (!userPath.isValid() || userPath.value().path().isEmpty()) {
        return QString();
    }

    QDBusInterface user(QStringLiteral("org.freedesktop.Accounts"),
        userPath.value().path(),
        QStringLiteral("org.freedesktop.Accounts.User"),
        QDBusConnection::systemBus());

    return user.isValid() ? LockAppearance::readableImage(
        user.property("IconFile").toString()) : QString();
}

User::User(QObject *parent)
    : QObject(parent)
    , m_isLogind(false)
    , m_isLock(false)
    , m_lockNum(4)
    , m_tryNum(5)
    , m_lockTimer(new QTimer)
{
    m_lockTimer->setInterval(1000 * 60);
    m_lockTimer->setSingleShot(false);
    connect(m_lockTimer.get(), &QTimer::timeout, this, &User::onLockTimeOut);
}

User::User(const User &user)
    : QObject(user.parent())
    , m_isLogind(user.m_isLogind)
    , m_isLock(user.m_isLock)
    , m_uid(user.m_uid)
    , m_lockNum(user.m_lockNum)
    , m_tryNum(user.m_tryNum)
    , m_userName(user.m_userName)
    , m_locale(user.m_locale)
    , m_lockTimer(user.m_lockTimer)
{

}

bool User::operator==(const User &user) const
{
    return type() == user.type() &&
            m_uid == user.m_uid;
}

void User::refreshLockBackgroundPath()
{
    emit lockBackgroundPathChanged(lockBackgroundPath());
}

void User::setLocale(const QString &locale)
{
    if (m_locale == locale) return;

    m_locale = locale;

    emit localeChanged(locale);
}

bool User::isNoPasswdGrp() const
{
    return checkUserIsNoPWGrp(this);
}

void User::setisLogind(bool isLogind) {
    if (m_isLogind == isLogind) {
        return;
    }

    m_isLogind = isLogind;

    emit logindChanged(isLogind);
}

void User::setPath(const QString &path)
{
    if (m_path == path) return;

    m_path = path;
}

bool User::isLockForNum()
{
    return m_isLock || --m_tryNum == 0;
}

void User::startLock()
{
    if (m_lockTimer->isActive()) return;

    m_isLock = true;

    onLockTimeOut();
}

void User::resetLock()
{
    m_tryNum = 5;
}

void User::onLockTimeOut()
{
    if (m_lockNum == 1) {
        m_lockTimer->stop();
        m_tryNum = 5;
        m_lockNum = 4;
        m_isLock = false;
    }
    else {
        m_lockNum--;
        m_lockTimer->start();
    }

    emit lockChanged(m_tryNum == 0);
}

NativeUser::NativeUser(const QString &userName, QObject *parent)
    : User(parent)
{
    // Source identity from the system password database (no dde-daemon).
    struct passwd *pw = getpwnam(userName.toUtf8().constData());
    if (pw) {
        m_userName = QString::fromLocal8Bit(pw->pw_name);
        m_uid = pw->pw_uid;
        const QString gecos = QString::fromLocal8Bit(pw->pw_gecos ? pw->pw_gecos : "");
        m_fullName = gecos.section(QLatin1Char(','), 0, 0);
    } else {
        m_userName = userName;
        m_uid = 0;
    }

    setPath(m_userName);
}

void NativeUser::setCurrentLayout(const QString &layout)
{
    if (m_currentLayout == layout)
        return;
    m_currentLayout = layout;
    emit currentKBLayoutChanged(layout);
}

QString NativeUser::displayName() const
{
    return m_fullName.isEmpty() ? name() : m_fullName;
}

QString NativeUser::avatarPath() const {
    const QString deepinAvatar = deepinAccountsAvatar(m_userName);
    if (!deepinAvatar.isEmpty()) {
        qInfo() << "(Frontend) Avatar: Using Deepin Accounts avatar for"
            << m_userName << deepinAvatar;
        return deepinAvatar;
    }

    const QString accountsAvatar = accountsServiceAvatar(m_userName);
    if (!accountsAvatar.isEmpty()) {
        qInfo() << "(Frontend) Avatar: Using AccountsService avatar for"
            << m_userName << accountsAvatar;
        return accountsAvatar;
    }

    const QString systemFace = LockAppearance::readableImage(
        QStringLiteral("/var/lib/AccountsService/icons/%1").arg(m_userName));
    if (!systemFace.isEmpty()) {
        return systemFace;
    }

    struct passwd* pw = getpwnam(m_userName.toUtf8().constData());
    if (pw && pw->pw_dir) {
        const QString home = QString::fromLocal8Bit(pw->pw_dir);
        const QString faceIcon = LockAppearance::readableImage(
            home + QStringLiteral("/.face.icon"));
        if (!faceIcon.isEmpty())
            return faceIcon;

        const QString face = LockAppearance::readableImage(
            home + QStringLiteral("/.face"));
        if (!face.isEmpty()) {
            return face;
        }
    }

    qWarning() << "(Frontend) Avatar: No readable avatar found for"
        << m_userName << "; using the default";
    return QStringLiteral(":/img/default_avatar.png");
}

QString NativeUser::greeterBackgroundPath() const
{
    // 显式配置了全局 Greeter 壁纸（state.conf）时优先使用；
    // 否则回退到用户锁屏壁纸链（与 lightdm greeter 行为一致，
    // override 文件全局可读，登录界面也能取到）。
    if (LockAppearance::hasConfiguredWallpaper())
        return LockAppearance::wallpaper();
    return lockBackgroundForUser(m_userName, m_uid);
}

QString NativeUser::lockBackgroundPath() const
{
    return lockBackgroundForUser(m_userName, m_uid);
}

QString NativeUser::desktopBackgroundPath() const
{
    return LockAppearance::wallpaper();
}

QStringList NativeUser::kbLayoutList()
{
    return QStringList();
}

QString NativeUser::currentKBLayout()
{
    return m_currentLayout;
}

bool NativeUser::isNoPasswdGrp() const
{
    return checkUserIsNoPWGrp(this);
}

ADDomainUser::ADDomainUser(uint uid, QObject *parent)
    : User(parent)
{
    m_uid = uid;
}

void ADDomainUser::setUserDisplayName(const QString &name)
{
    if (m_displayName == name) {
        return;
    }

    m_displayName = name;

    emit displayNameChanged(name);
}

void ADDomainUser::setUserName(const QString &name)
{
    if (m_userName == name) {
        return;
    }

    m_userName = name;
}

QString ADDomainUser::displayName() const
{
    return m_displayName.isEmpty() ? m_userName : m_displayName;
}

QString ADDomainUser::avatarPath() const
{
    return QString(":/img/default_avatar.png");
}

QString ADDomainUser::greeterBackgroundPath() const
{
    // 与 NativeUser 一致：全局壁纸未显式配置时回退到用户锁屏壁纸链。
    if (LockAppearance::hasConfiguredWallpaper())
        return LockAppearance::wallpaper();
    return lockBackgroundForUser(m_userName, m_uid);
}

QString ADDomainUser::lockBackgroundPath() const
{
    return lockBackgroundForUser(m_userName, m_uid);
}

QString ADDomainUser::desktopBackgroundPath() const
{
    return LockAppearance::wallpaper();
}
