#ifndef USERINFO_H
#define USERINFO_H

#include <QObject>
#include <QTimer>

#include <memory>

// The original GXDE greeter depends on DDE-Daemon, now sourcing from system
// password databases, removing dde-daemon requirements...
// 原版GXDE greeter依赖dde-daemon获取账户信息，现在我们自己读本地账户

class User : public QObject
{
    Q_OBJECT

public:
    enum UserType
    {
        Native,
        ADDomain,
    };

    User(QObject *parent);
    User(const User &user);

signals:
    void displayNameChanged(const QString &displayname) const;
    void logindChanged(bool islogind) const;
    void avatarChanged(const QString &avatar) const;
    void lockBackgroundPathChanged(const QString &path) const;
    void desktopBackgroundPathChanged(const QString &path) const;
    void localeChanged(const QString &locale) const;
    void kbLayoutListChanged(const QStringList &list);
    void currentKBLayoutChanged(const QString &layout);
    void lockChanged(bool lock);

public:
    bool operator==(const User &user) const;
    const QString name() const { return m_userName; }

    bool isLogin() const { return m_isLogind; }
    uint uid() const { return m_uid; }

    const QString locale() const { return m_locale; }
    void setLocale(const QString &locale);

    virtual bool isNoPasswdGrp() const;

    void setisLogind(bool isLogind);
    virtual void setCurrentLayout(const QString &layout) { Q_UNUSED(layout); }

    void setPath(const QString &path);
    const QString path() const { return m_path; }
    void refreshLockBackgroundPath();

    uint lockNum() const { return m_lockNum; }
    bool isLock() const { return m_isLock; }
    bool isLockForNum();
    void startLock();
    void resetLock();

    virtual UserType type() const = 0;
    virtual QString displayName() const { return m_userName; }
    virtual QString avatarPath() const = 0;
    virtual QString greeterBackgroundPath() const = 0;
    virtual QString lockBackgroundPath() const = 0;
    virtual QString desktopBackgroundPath() const = 0;
    virtual QStringList kbLayoutList() { return QStringList(); }
    virtual QString currentKBLayout() { return QString(); }

protected:
    void onLockTimeOut();

protected:
    bool m_isLogind;
    bool m_isLock;
    uint m_uid;
    uint m_lockNum; // minute basis
    uint m_tryNum; // try number
    QString m_userName;
    QString m_locale;
    QString m_path;
    std::shared_ptr<QTimer> m_lockTimer;
};

class NativeUser : public User
{
    Q_OBJECT

public:
    explicit NativeUser(const QString &userName, QObject *parent = nullptr);

    void setCurrentLayout(const QString &currentKBLayout) override;

    UserType type() const override { return Native; }
    QString displayName() const override;
    QString avatarPath() const override;
    QString greeterBackgroundPath() const override;
    QString lockBackgroundPath() const override;
    QString desktopBackgroundPath() const override;
    QStringList kbLayoutList() override;
    QString currentKBLayout() override;
    bool isNoPasswdGrp() const override;

private:
    QString m_fullName;       // GECOS display name, could be empty
    QString m_currentLayout;
};

class ADDomainUser : public User
{
    Q_OBJECT

public:
    ADDomainUser(uint uid, QObject *parent = nullptr);

    void setUserDisplayName(const QString &name);
    void setUserName(const QString &name);

    QString displayName() const override;
    UserType type() const override { return ADDomain; }
    QString avatarPath() const override;
    QString greeterBackgroundPath() const override;
    QString lockBackgroundPath() const override;
    QString desktopBackgroundPath() const override;

private:
    QString m_displayName;
};

#endif // USERINFO_H
