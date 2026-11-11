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

#ifndef SRC_LOCK_CLASSIC_LOCK_WAYLANDSESSIONLOCK_H_
#define SRC_LOCK_CLASSIC_LOCK_WAYLANDSESSIONLOCK_H_

#include <QtWaylandClient/qwaylandclientextension.h>
#include <QtWaylandClient/private/qwaylandshellintegration_p.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QSize>

#include "wayland-ext-session-lock-v1-client-protocol.h"

class QScreen;
class QWindow;
class WaylandSessionLockSurface;

class WaylandSessionLockIntegration : public QWaylandClientExtension,
        public QtWaylandClient::QWaylandShellIntegration {
    Q_OBJECT

public:
    explicit WaylandSessionLockIntegration(QObject* parent = nullptr);
    ~WaylandSessionLockIntegration() override;

    bool initialize(QtWaylandClient::QWaylandDisplay* display) override;
    QtWaylandClient::QWaylandShellSurface* createShellSurface(
        QtWaylandClient::QWaylandWindow* window) override;

    const struct wl_interface* extensionInterface() const override;
    void bind(struct wl_registry* registry, int id, int version) override;

    // Lock surfaces are bound to |screen|'s output. Qt may move a window to a
    // different QScreen on its own (it follows the window position, and
    // mirrored outputs share one), so the window's screen is only a fallback.
    bool installOnWindow(QWindow* window, QScreen* screen = nullptr);
    bool isAvailable() const;
    bool isSessionLocked() const;
    bool hasLockRequest() const;

    struct ext_session_lock_v1* ensureLock();
    bool unlock();
    void handleLocked();
    void handleFinished();

Q_SIGNALS:
    void locked();
    void unlocked();
    void finished();

private:
    struct wl_output* outputForWindow(
        QtWaylandClient::QWaylandWindow* window) const;
    bool registerSurface(WaylandSessionLockSurface* surface,
        struct wl_output* output);
    void unregisterSurface(WaylandSessionLockSurface* surface);
    void releaseSurfaces();
    struct ext_session_lock_manager_v1* m_manager = nullptr;
    struct ext_session_lock_v1* m_lock = nullptr;
    QSet<WaylandSessionLockSurface *> m_surfaces;
    QHash<struct wl_output*, WaylandSessionLockSurface *> m_outputSurfaces;
    QHash<QWindow *, QPointer<QScreen>> m_targetScreens;
    bool m_locked = false;
    bool m_unlockPending = false;

    friend class WaylandSessionLockSurface;
};

class WaylandSessionLockSurface : public QtWaylandClient::QWaylandShellSurface {
public:
    WaylandSessionLockSurface(WaylandSessionLockIntegration* integration,
        QtWaylandClient::QWaylandWindow* window);
    ~WaylandSessionLockSurface() override;

    bool isValid() const;
    bool isExposed() const override;
    void applyConfigure() override;
    void setWindowGeometry(const QRect& rect) override;
    void setWindowPosition(const QPoint& position) override;
    void requestWindowStates(Qt::WindowStates states) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    std::any surfaceRole() const override;

    void release();
    static void handleConfigureCallback(
        void* data,
        struct ext_session_lock_surface_v1* surface,
        uint32_t serial,
        uint32_t width,
        uint32_t height);

private:
    void bindLockSurface();
    void handleConfigure(uint32_t serial, uint32_t width, uint32_t height);

    WaylandSessionLockIntegration* m_integration = nullptr;
    struct ext_session_lock_surface_v1* m_surface = nullptr;
    struct wl_output* m_output = nullptr;
    QSize m_pendingSize;
    uint32_t m_pendingConfigureSerial = 0;
    uint32_t m_appliedConfigureSerial = 0;
    bool m_bindingPending = false;
    bool m_configured = false;
    friend class WaylandSessionLockIntegration;
};

#endif  // SRC_LOCK_CLASSIC_LOCK_WAYLANDSESSIONLOCK_H_
