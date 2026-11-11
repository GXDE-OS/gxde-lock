#include "multiscreenmanager.h"

#include <QApplication>
#include <QDebug>

MultiScreenManager::MultiScreenManager(QObject *parent)
    : QObject(parent)
    , m_registerFunction(nullptr)
    , m_raiseContentFrameTimer(new QTimer(this))
{
    // Additions may be deferred until Qt has finished publishing the new
    // QScreen. Removals must be handled synchronously so a Wayland lock
    // surface is destroyed before Qt can hide/reuse its window.
    connect(qApp, &QGuiApplication::screenAdded, this, &MultiScreenManager::onScreenAdded, Qt::QueuedConnection);
    connect(qApp, &QGuiApplication::screenRemoved, this, &MultiScreenManager::onScreenRemoved, Qt::DirectConnection);

    // 在sw平台存在复制模式显示问题，使用延迟来置顶一个Frame
    m_raiseContentFrameTimer->setInterval(50);
    m_raiseContentFrameTimer->setSingleShot(true);

    connect(m_raiseContentFrameTimer, &QTimer::timeout, this, &MultiScreenManager::raiseContentFrame);
}

MultiScreenManager::~MultiScreenManager()
{
    const auto frames = m_frames;
    m_frames.clear();
    for (QWidget *frame : frames) {
        delete frame;
    }
}

void MultiScreenManager::register_for_mutil_screen(std::function<QWidget *(QScreen *)> function)
{
    m_registerFunction = function;

    // update all screen
    for (QScreen *screen : qApp->screens()) {
        onScreenAdded(screen);
    }
}

void MultiScreenManager::startRaiseContentFrame()
{
    m_raiseContentFrameTimer->start();
}

void MultiScreenManager::recreateFrames()
{
    if (!m_registerFunction) {
        return;
    }

    m_raiseContentFrameTimer->stop();

    const auto oldFrames = m_frames;
    m_frames.clear();
    for (QWidget *frame : oldFrames) {
        delete frame;
    }

    for (QScreen *screen : qApp->screens()) {
        onScreenAdded(screen);
    }
}

void MultiScreenManager::onScreenAdded(QScreen *screen)
{
    if (!m_registerFunction || !screen || m_frames.contains(screen)) {
        return;
    }

    QWidget *frame = m_registerFunction(screen);
    if (!frame) {
        return;
    }
    m_frames.insert(screen, frame);

    startRaiseContentFrame();
}

void MultiScreenManager::onScreenRemoved(QScreen *screen)
{
    if (!m_registerFunction) {
        return;
    }

    if (QWidget *frame = m_frames.take(screen)) {
        delete frame;
    }

    startRaiseContentFrame();
}

void MultiScreenManager::raiseContentFrame()
{
    for (auto it = m_frames.constBegin(); it != m_frames.constEnd(); ++it) {
        if (it.value() && it.value()->property("contentVisible").toBool()) {
            it.value()->raise();
            return;
        }
    }
}
