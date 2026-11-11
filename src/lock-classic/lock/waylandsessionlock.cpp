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

#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtGui/qscreen_platform.h>
#include <wayland-client.h>

#include <algorithm>

#include <QDebug>
#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QTimer>
#include <QWindow>

#include "waylandsessionlock.h"

namespace {

constexpr int kSessionLockProtocolVersion = 1;

const ext_session_lock_v1_listener kLockListener = {
    [](void* data, ext_session_lock_v1* lock) {
        Q_UNUSED(lock)
        static_cast<WaylandSessionLockIntegration *>(data)->handleLocked();
    },
    [](void* data, ext_session_lock_v1* lock) {
        Q_UNUSED(lock)
        static_cast<WaylandSessionLockIntegration *>(data)->handleFinished();
    },
};

const ext_session_lock_surface_v1_listener kSurfaceListener = {
    WaylandSessionLockSurface::handleConfigureCallback,
};

wl_output* nativeOutputForWindow(QtWaylandClient::QWaylandWindow* window) {
    if (!window || !window->window() || !window->window()->screen()) {
        return nullptr;
    }

    QScreen* screen = window->window()->screen();
    auto* waylandScreen =
        screen->nativeInterface<QNativeInterface::QWaylandScreen>();
    return waylandScreen ? waylandScreen->output() : nullptr;
}

QSize fallbackSurfaceSize(QtWaylandClient::QWaylandWindow* window) {
    if (!window || !window->window() || !window->window()->screen()) {
        return QSize();
    }

    return window->window()->screen()->geometry().size();
}

}  // namespace

WaylandSessionLockIntegration::WaylandSessionLockIntegration(
        QObject* parent) : QWaylandClientExtension(
            kSessionLockProtocolVersion) {
    setParent(parent);
}

WaylandSessionLockIntegration::~WaylandSessionLockIntegration() {
    releaseSurfaces();

    if (m_lock && !m_locked) {
        ext_session_lock_v1_destroy(m_lock);
    } else if (m_lock) {
        qWarning()
            << "(Lock) Check: Wl session lock obj still locked at shutdown!!";
    }

    if (m_manager) {
        ext_session_lock_manager_v1_destroy(m_manager);
    }
}

bool WaylandSessionLockIntegration::initialize(
        QtWaylandClient::QWaylandDisplay* display) {
    Q_UNUSED(display)
    QWaylandClientExtension::initialize();
    return isAvailable();
}

QtWaylandClient::QWaylandShellSurface*
        WaylandSessionLockIntegration::createShellSurface(
        QtWaylandClient::QWaylandWindow* window) {
    if (!isAvailable()) {
        qWarning() << "(Lock) Fatal: ext-session-lock-v1 is not available!!";
        return nullptr;
    }

    auto* surface = new WaylandSessionLockSurface(this, window);
    if (!surface->isValid()) {
        delete surface;
        return nullptr;
    }

    return surface;
}

const wl_interface* WaylandSessionLockIntegration::extensionInterface() const {
    return &ext_session_lock_manager_v1_interface;
}

void WaylandSessionLockIntegration::bind(wl_registry *registry, int id,
        int version) {
    const int bindVersion = std::min(version, kSessionLockProtocolVersion);
    setVersion(bindVersion);
    m_manager = static_cast<ext_session_lock_manager_v1 *>(
        wl_registry_bind(registry, static_cast<uint32_t>(id),
            &ext_session_lock_manager_v1_interface,
            static_cast<uint32_t>(bindVersion)));
}

bool WaylandSessionLockIntegration::installOnWindow(QWindow* window) {
    if (!isAvailable() || !window) {
        return false;
    }

    if (!window->handle()) {
        window->create();
    }

    auto* waylandWindow =
        dynamic_cast<QtWaylandClient::QWaylandWindow *>(window->handle());

    if (!waylandWindow) {
        qWarning() << "(Lock) Fatal: Window is not a Qt Wayland window!!";
        return false;
    }

    if (waylandWindow->shellSurface()) {
        qWarning() << "(Lock) Fatal: Wayland shell surface already exists!!"
            "CANNOT switch this window to ext-session-lock-v1.";
        return false;
    }

    waylandWindow->setShellIntegration(this);
    return true;
}

bool WaylandSessionLockIntegration::isAvailable() const {
    return isActive() && m_manager;
}

bool WaylandSessionLockIntegration::isSessionLocked() const {
    return m_locked;
}

bool WaylandSessionLockIntegration::hasLockRequest() const {
    return m_lock;
}

ext_session_lock_v1* WaylandSessionLockIntegration::ensureLock() {
    if (m_lock) {
        return m_lock;
    }

    if (!isAvailable()) {
        return nullptr;
    }

    m_locked = false;
    m_unlockPending = false;
    m_lock = ext_session_lock_manager_v1_lock(m_manager);
    if (!m_lock) {
        qWarning()
            << "(Lock) Fatal: Failed to request ext-session-lock-v1 object.";
        return nullptr;
    }

    ext_session_lock_v1_add_listener(m_lock, &kLockListener, this);
    return m_lock;
}

bool WaylandSessionLockIntegration::unlock() {
    if (!m_lock) {
        return true;
    }

    if (!m_locked) {
        m_unlockPending = true;
        return false;
    }

    ext_session_lock_v1_unlock_and_destroy(m_lock);
    m_lock = nullptr;
    m_locked = false;
    m_unlockPending = false;
    releaseSurfaces();
    Q_EMIT unlocked();
    return true;
}

bool WaylandSessionLockIntegration::registerSurface(
        WaylandSessionLockSurface* surface, wl_output* output) {
    if (!surface || !output) {
        return false;
    }

    WaylandSessionLockSurface* existing = m_outputSurfaces.value(output);
    if (existing && existing != surface) {
        return false;
    }

    m_outputSurfaces.insert(output, surface);
    m_surfaces.insert(surface);
    return true;
}

void WaylandSessionLockIntegration::unregisterSurface(
    WaylandSessionLockSurface* surface) {
    if (surface && surface->m_output
            && m_outputSurfaces.value(surface->m_output) == surface) {
        m_outputSurfaces.remove(surface->m_output);
    }
    m_surfaces.remove(surface);
}

void WaylandSessionLockIntegration::releaseSurfaces() {
    const auto surfaces = m_surfaces;
    m_surfaces.clear();
    m_outputSurfaces.clear();
    for (WaylandSessionLockSurface* surface : surfaces) {
        if (surface) {
            surface->release();
        }
    }
}

void WaylandSessionLockIntegration::handleLocked() {
    m_locked = true;
    Q_EMIT locked();

    if (m_unlockPending) {
        unlock();
    }
}

void WaylandSessionLockIntegration::handleFinished() {
    if (m_lock) {
        if (m_locked) {
            ext_session_lock_v1_unlock_and_destroy(m_lock);
        } else {
            ext_session_lock_v1_destroy(m_lock);
        }
    }

    m_lock = nullptr;
    m_locked = false;
    m_unlockPending = false;
    releaseSurfaces();
    Q_EMIT finished();
}

WaylandSessionLockSurface::WaylandSessionLockSurface(
        WaylandSessionLockIntegration* integration,
        QtWaylandClient::QWaylandWindow* window)
            : QtWaylandClient::QWaylandShellSurface(window)
                , m_integration(integration) {
    if (!m_integration) {
        return;
    }

    ext_session_lock_v1* lock = m_integration->ensureLock();
    wl_surface* surface = wlSurface();
    wl_output* output = nativeOutputForWindow(window);
    if (!lock || !surface || !output) {
        qWarning()
            << "(Lock) Fatal: Cannot create ext-session-lock-v1 surface!!";
        return;
    }

    m_bindingPending = true;
    // QWaylandWindow commits once while finishing window initialization.
    // Bind the lock role afterwards to avoid the NONNULL issue.
    // That drove me MAD...
    QTimer::singleShot(0, this, [this] { bindLockSurface(); });
}

void WaylandSessionLockSurface::bindLockSurface() {
    if (!m_bindingPending || !m_integration) {
        return;
    }

    m_bindingPending = false;
    ext_session_lock_v1* lock = m_integration->ensureLock();
    wl_surface* surface = wlSurface();
    auto* waylandWindow =
        static_cast<QtWaylandClient::QWaylandWindow *>(window());
    wl_output* output = nativeOutputForWindow(waylandWindow);
    if (!lock || !surface || !output) {
        qWarning()
            << "(Lock) Fatal: Cannot create ext-session-lock-v1 surface!!";
        return;
    }

    if (!m_integration->registerSurface(this, output)) {
        qWarning()
            << "(Lock) Refusing duplicate ext-session-lock-v1 surface for"
            << window()->window()->screen();
        return;
    }
    m_output = output;
    m_surface = ext_session_lock_v1_get_lock_surface(lock, surface, output);
    if (!m_surface) {
        m_integration->unregisterSurface(this);
        m_output = nullptr;
        qWarning()
            << "(Lock) Fatal: Failed to bind ext-session-lock-v1 surface!!";
        return;
    }

    ext_session_lock_surface_v1_add_listener(
        m_surface, &kSurfaceListener, this);
}

WaylandSessionLockSurface::~WaylandSessionLockSurface() {
    release();

    if (m_integration) {
        m_integration->unregisterSurface(this);
    }
}

bool WaylandSessionLockSurface::isValid() const {
    return m_bindingPending || m_surface;
}

bool WaylandSessionLockSurface::isExposed() const {
    return m_configured;
}

void WaylandSessionLockSurface::applyConfigure() {
    if (!m_surface || m_pendingConfigureSerial == m_appliedConfigureSerial) {
        return;
    }

    const bool wasExposed = isExposed();
    if (!m_pendingSize.isEmpty()) {
        resizeFromApplyConfigure(m_pendingSize);
        m_pendingSize = QSize();
    }

    // Ack only after Qt has applied the matching size. An ack makes the very
    // next wl_surface commit subject to ext-session-lock's exact-dimensions
    // rule, so acking in the protocol callback races normal Qt repainting.
    m_appliedConfigureSerial = m_pendingConfigureSerial;
    m_configured = true;
    ext_session_lock_surface_v1_ack_configure(
        m_surface, m_appliedConfigureSerial);
    if (!wasExposed) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        window()->updateExposure();
#else
        window()->sendRecursiveExposeEvent();
#endif
    }
}

void WaylandSessionLockSurface::setWindowGeometry(const QRect& rect) {
    Q_UNUSED(rect)
}

void WaylandSessionLockSurface::setWindowPosition(const QPoint& position) {
    Q_UNUSED(position)
}

void WaylandSessionLockSurface::requestWindowStates(Qt::WindowStates states) {
    Q_UNUSED(states)
}

void WaylandSessionLockSurface::setWindowFlags(Qt::WindowFlags flags) {
    Q_UNUSED(flags)
}

std::any WaylandSessionLockSurface::surfaceRole() const {
    return m_surface;
}

void WaylandSessionLockSurface::release() {
    m_bindingPending = false;
    if (!m_surface) {
        return;
    }

    ext_session_lock_surface_v1_destroy(m_surface);
    m_surface = nullptr;
    m_configured = false;
    m_pendingSize = QSize();
    m_pendingConfigureSerial = 0;
    m_appliedConfigureSerial = 0;
}

void WaylandSessionLockSurface::handleConfigure(uint32_t serial,
        uint32_t width, uint32_t height) {
    if (!m_surface) {
        return;
    }

    QSize size(static_cast<int>(width), static_cast<int>(height));
    if (size.isEmpty()) {
        size = fallbackSurfaceSize(
            static_cast<QtWaylandClient::QWaylandWindow *>(window()));
    }

    m_pendingConfigureSerial = serial;
    m_pendingSize = size;
    if (!m_configured) {
        // The first configure exposes the window, so it must be applied
        // immediately. Later resizes are deferred until Qt is not painting.
        applyConfigure();
    } else {
        applyConfigureWhenPossible();
    }
}

void WaylandSessionLockSurface::handleConfigureCallback(void* data,
        ext_session_lock_surface_v1* surface, uint32_t serial, uint32_t width,
        uint32_t height) {
    Q_UNUSED(surface)
    static_cast<WaylandSessionLockSurface *>(data)->handleConfigure(
        serial, width, height);
}
