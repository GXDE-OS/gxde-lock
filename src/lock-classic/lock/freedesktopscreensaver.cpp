#include "freedesktopscreensaver.h"

#include "sessionbasemodel.h"

FreedesktopScreenSaver::FreedesktopScreenSaver(QObject *parent, SessionBaseModel *model)
    : QDBusAbstractAdaptor(parent)
    , m_model(model)
{
}

void FreedesktopScreenSaver::Lock()
{
    m_model->setIsShow(true);
    emit m_model->visibleChanged(true);
}

bool FreedesktopScreenSaver::GetActive()
{
    return m_model->isShow();
}

bool FreedesktopScreenSaver::SetActive(bool state)
{
    m_model->setIsShow(state);
    emit m_model->visibleChanged(state);
    return true;
}

quint32 FreedesktopScreenSaver::GetActiveTime()
{
    return 0;
}

quint32 FreedesktopScreenSaver::GetSessionIdleTime()
{
    return 0;
}

void FreedesktopScreenSaver::SimulateUserActivity()
{
}

quint32 FreedesktopScreenSaver::Inhibit(const QString &application_name, const QString &reason)
{
    Q_UNUSED(application_name)
    Q_UNUSED(reason)
    return 0;
}

void FreedesktopScreenSaver::UnInhibit(quint32 cookie)
{
    Q_UNUSED(cookie)
}
