#ifndef FREEDESKTOPSCREENSAVER_H
#define FREEDESKTOPSCREENSAVER_H

#include <QDBusAbstractAdaptor>

class SessionBaseModel;

// Exposes the org.freedesktop.ScreenSaver interface so cross-desktop "Lock
// Screen" actions trigger gxde-lock. libxfce4ui (XFCE), GNOME, Cinnamon
// and MATE all probe this name and call Lock() over D-Bus when it is owned;
// otherwise XFCE falls back to light-locker / blanking the X screen (the black
// screen we were seeing).
class FreedesktopScreenSaver : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ScreenSaver")
public:
    explicit FreedesktopScreenSaver(QObject *parent, SessionBaseModel *model);

public Q_SLOTS:
    void Lock();
    bool GetActive();
    bool SetActive(bool state);
    quint32 GetActiveTime();
    quint32 GetSessionIdleTime();
    void SimulateUserActivity();
    quint32 Inhibit(const QString &application_name, const QString &reason);
    void UnInhibit(quint32 cookie);

Q_SIGNALS:
    void ActiveChanged(bool state);

private:
    SessionBaseModel *m_model;
};

#endif  // FREEDESKTOPSCREENSAVER_H
