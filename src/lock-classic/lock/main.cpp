/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <DLog>

#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>

#include "lockframe.h"
#include "lockshortcutmanager.h"
#include "dbus/dbuslockfrontservice.h"
#include "dbus/dbuslockagent.h"
// PATCHS
#include "freedesktopscreensaver.h"
// PATCHE
#include "multiscreenmanager.h"
#include "waylandsessionlock.h"

#include "lockcontent.h"
#include "lockworker.h"
#include "sessionbasemodel.h"
#include "propertygroup.h"

#include "Constants.h"
#include "lockstyle.h"
#include "lockstylemanager.h"

#include <QLabel>
#include <QFile>
#include <QFileInfo>
#include <QScreen>
#include <QWindow>
#include <dapplication.h>
#include <QDBusInterface>
#include <QTimer>
#include <QtGui/qguiapplication_platform.h>
#include <wayland-client.h>

#include <memory>
// PATCHS
// Qt6: removed `#include <QDesktopWidget>` (QDesktopWidget no longer exists)
// PATCHE

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

namespace {

const QString kGxdeLockService =
    QStringLiteral("top.gxde.Lock");
const QString kGxdeLockPath =
    QStringLiteral("/top/gxde/Lock");
const QString kGxdeLockInterface =
    QStringLiteral("top.gxde.Lock");

bool shouldUseX11(int argc, char *argv[])
{
    QByteArray requestedPlatform = qgetenv("QT_QPA_PLATFORM");
    for (int i = 1; i + 1 < argc; ++i) {
        if (QByteArray(argv[i]) == "-platform") {
            requestedPlatform = argv[i + 1];
        }
    }

    return requestedPlatform.startsWith("xcb")
        || requestedPlatform.startsWith("dxcb")
        || (requestedPlatform.isEmpty()
            && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY"));
}

// The handoff runs before QApplication exists, so QCoreApplication's
// applicationDirPath() is unavailable. /proc/self/exe is Linux-specific but
// this locker already depends on Linux/FreeBSD session APIs; on failure we
// simply keep the classic frontend.
QString executableDirectory()
{
    const QFileInfo self(QStringLiteral("/proc/self/exe"));
    const QString target = self.symLinkTarget();
    if (!target.isEmpty()) {
        return QFileInfo(target).absolutePath();
    }
    return QFileInfo(self.canonicalFilePath()).absolutePath();
}

bool shouldHandOffToNeo(int argc, char *argv[])
{
    if (qEnvironmentVariableIsSet("GXDE_LOCK_NO_HANDOFF")) {
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        const QByteArray argument(argv[i]);
        if (argument == "--help" || argument == "-h"
            || argument == "--version" || argument == "-v") {
            return false;
        }
    }

    const QString lockConfig = qEnvironmentVariable(
        "GXDE_LOCK_CONF", QStringLiteral(CONFIG_FILE));
    const QString omgConfig = qEnvironmentVariable(
        "GXDE_LOCK_OMG_CONF", QStringLiteral(OMG_CONFIG_FILE));

    const gxde::lock::LockStyle style =
        gxde::lock::LockStyleResolver::ResolveFromFiles(
            lockConfig, omgConfig, executableDirectory());
    return style == gxde::lock::LockStyle::kModern;
}

void execNeo(int argc, char *argv[])
{
    const QString path = gxde::lock::LockStyleResolver::ExecutablePath(
        gxde::lock::LockStyle::kModern, executableDirectory());

    qInfo() << "Handing off to the QML lock screen:" << path;

    QList<QByteArray> arguments;
    for (int i = 1; i < argc; ++i) {
        arguments << QByteArray(argv[i]);
    }
    gxde::lock::ExecLockScreen(path, arguments);
}

void roundtripWaylandDisplay() {
    auto* native =
        qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (!native || !native->display()) {
        return;
    }

    wl_display_roundtrip(native->display());
}

void configureLayerShell(LockFrame* lockFrame) {
    LayerShellQt::Window *layerWindow =
        LayerShellQt::Window::get(lockFrame->windowHandle());
    layerWindow->setScreenConfiguration(
        LayerShellQt::Window::ScreenFromQWindow);
    layerWindow->setScope(QStringLiteral("gxde-lock"));
    layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
    LayerShellQt::Window::Anchors anchors;
    anchors.setFlag(LayerShellQt::Window::AnchorTop);
    anchors.setFlag(LayerShellQt::Window::AnchorBottom);
    anchors.setFlag(LayerShellQt::Window::AnchorLeft);
    anchors.setFlag(LayerShellQt::Window::AnchorRight);
    layerWindow->setAnchors(anchors);
    layerWindow->setExclusiveZone(-1);
    layerWindow->setMargins(QMargins());
    layerWindow->setKeyboardInteractivity(
        LayerShellQt::Window::KeyboardInteractivityExclusive);
    layerWindow->setCloseOnDismissed(false);
}

}  // namespace

int main(int argc, char *argv[])
{
    if (shouldHandOffToNeo(argc, argv)) {
        execNeo(argc, argv);
        // execNeo() only returns when the new binary could not be started;
        // keep the classic locker as a safe fallback.
    }

    const bool useX11 = shouldUseX11(argc, argv);
    if (useX11) {
        DApplication::loadDXcbPlugin();
    } else {
        LayerShellQt::Shell::useLayerShell();
    }

    DApplication app(argc, argv);
    // The daemon intentionally has no visible windows between lock cycles.
    app.setQuitOnLastWindowClosed(false);
    app.setOrganizationName("deepin");
    app.setApplicationName("gxde-lock-classic");
    app.setApplicationVersion("2015.1.0");

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    QTranslator translator;
    translator.load("/usr/share/gxde-session-ui/translations/gxde-session-ui_" + QLocale::system().name());
    app.installTranslator(&translator);

    QCommandLineParser cmdParser;
    cmdParser.addHelpOption();
    cmdParser.addVersionOption();

    QCommandLineOption backend(QStringList() << "d" << "daemon", "start to daemon mode");
    cmdParser.addOption(backend);
    QCommandLineOption switchUser(QStringList() << "s" << "switch", "show user switch");
    cmdParser.addOption(switchUser);
    QCommandLineOption autoEnroll(
        QStringLiteral("auto-enroll"),
        QStringLiteral("enroll the lock shortcut (enabled by default)"));
    cmdParser.addOption(autoEnroll);
    cmdParser.process(app);

    const bool runDaemon = cmdParser.isSet(backend);
    const bool showUserList = cmdParser.isSet(switchUser);

    if (!app.setSingleInstance(
            QStringLiteral("gxde-lock-classic"), DApplication::UserScope)) {
        if (!runDaemon) {
            if (showUserList) {
                QDBusInterface lockFront(
                    DBUS_NAME,
                    DBUS_PATH,
                    QStringLiteral("com.deepin.dde.lockFront"),
                    QDBusConnection::sessionBus());
                lockFront.asyncCall(QStringLiteral("ShowUserList"));
            } else {
                QDBusInterface displayManager(
                    kGxdeLockService,
                    kGxdeLockPath,
                    kGxdeLockInterface,
                    QDBusConnection::sessionBus());
                displayManager.asyncCall(QStringLiteral("Show"));
            }
        }
        return 0;
    }

    SessionBaseModel *model = new SessionBaseModel(SessionBaseModel::AuthType::LockType);
    LockWorker *worker = new LockWorker(model); //
    PropertyGroup *property_group = new PropertyGroup(worker);

    property_group->addProperty("contentVisible");

    std::unique_ptr<WaylandSessionLockIntegration> sessionLockIntegration;
    if (!useX11) {
        sessionLockIntegration =
            std::make_unique<WaylandSessionLockIntegration>(&app);
        sessionLockIntegration->initialize(nullptr);
        roundtripWaylandDisplay();

        if (sessionLockIntegration->isAvailable()) {
            qInfo() << "(Lock) Protocol: Using ext-session-lock-v1.";
            QObject::connect(sessionLockIntegration.get(),
                    &WaylandSessionLockIntegration::locked,
                    &app, [] {
                qInfo() << "(Lock) Status: Wayland session is locked.";
            });

            QObject::connect(sessionLockIntegration.get(),
                    &WaylandSessionLockIntegration::finished,
                    model, [model] {
                qWarning() << "(Lock) Status: Wayland session lock was finished"
                    " by WM.";
                model->setIsShow(false);
            });

            QObject::connect(model,
                    &SessionBaseModel::authFinished,
                    sessionLockIntegration.get(),
                    [lock = sessionLockIntegration.get()](bool ok) {
                if (ok) {
                    lock->unlock();
                }
            });
        } else {
            qWarning()
                << "(Lock) Warning: ext-session-lock-v1 is unavailable;"
                   " using LayerShell fallback.";
        }
    }

    auto createFrame = [&] (QScreen *screen) -> QWidget* {
        LockFrame *lockFrame = new LockFrame(model);
        lockFrame->setScreen(screen);
        if (!useX11) {
            const bool usingSessionLock =
                sessionLockIntegration
                && sessionLockIntegration->isAvailable()
                && sessionLockIntegration->installOnWindow(
                    lockFrame->windowHandle());
            if (!usingSessionLock) {
                configureLayerShell(lockFrame);
            }
        }
        property_group->addObject(lockFrame);
        QObject::connect(lockFrame, &LockFrame::requestSwitchToUser, worker, &LockWorker::switchToUser);
        QObject::connect(lockFrame, &LockFrame::requestAuthUser, worker, &LockWorker::authUser);
        QObject::connect(model, &SessionBaseModel::visibleChanged, lockFrame, &LockFrame::setVisible);
        QObject::connect(model, &SessionBaseModel::showUserList, lockFrame, &LockFrame::showUserList);
        QObject::connect(lockFrame, &LockFrame::requestSetLayout, worker, &LockWorker::setLayout);
        QObject::connect(lockFrame, &LockFrame::requestEnableHotzone, worker, &LockWorker::enableZoneDetected, Qt::UniqueConnection);
        QObject::connect(lockFrame, &LockFrame::destroyed, property_group, &PropertyGroup::removeObject);
        lockFrame->setVisible(model->isShow());
        return lockFrame;
    };

    MultiScreenManager multi_screen_manager;
    multi_screen_manager.register_for_mutil_screen(createFrame);

    if (sessionLockIntegration && sessionLockIntegration->isAvailable()) {
        // ext-session-lock roles are tied to a wl_surface for life. Recreate
        // the hidden frames after every completed/aborted lock cycle so a
        // later request receives new surfaces instead of reusing dead roles.
        QObject::connect(sessionLockIntegration.get(),
                &WaylandSessionLockIntegration::unlocked,
                &multi_screen_manager,
                &MultiScreenManager::recreateFrames,
                Qt::QueuedConnection);
        QObject::connect(sessionLockIntegration.get(),
                &WaylandSessionLockIntegration::finished,
                &multi_screen_manager,
                &MultiScreenManager::recreateFrames,
                Qt::QueuedConnection);
    }

    QObject::connect(model, &SessionBaseModel::visibleChanged, &multi_screen_manager, &MultiScreenManager::startRaiseContentFrame);

    DBusLockAgent agent;
    agent.setModel(model);
    DBusLockFrontService service(&agent);
    Q_UNUSED(service);
    // PATCHS
    // Also expose org.freedesktop.ScreenSaver so cross-desktop "Lock Screen"
    // actions (XFCE/GNOME/Cinnamon/MATE probe this name) trigger gxde-lock.
    FreedesktopScreenSaver screenSaver(&agent, model);
    Q_UNUSED(screenSaver);
    // PATCHE

    LockShortcutManager shortcutManager(model);
    GxdeLockService displayManagerService(&shortcutManager);
    Q_UNUSED(displayManagerService)

    QDBusConnection conn = QDBusConnection::sessionBus();
    if (!conn.registerService(kGxdeLockService)
        || !conn.registerObject(
            kGxdeLockPath,
            &shortcutManager,
            QDBusConnection::ExportAdaptors)) {
        qWarning() << "Failed to register"
                   << kGxdeLockService << conn.lastError();
        return 1;
    }

    // Compatibility names are best-effort. The GXDE interface and shortcut
    // remain usable even when another locker already owns one of these names.
    if (conn.registerService(DBUS_NAME)) {
        conn.registerObject(
            QStringLiteral("/com/deepin/dde/lockFront"), &agent);
    } else {
        qInfo() << "com.deepin.dde.lockFront is already owned.";
    }

    if (conn.registerService(QStringLiteral("org.freedesktop.ScreenSaver"))) {
        conn.registerObject(
            QStringLiteral("/org/freedesktop/ScreenSaver"), &agent);
        conn.registerObject(QStringLiteral("/ScreenSaver"), &agent);
    }

    QTimer::singleShot(0, &shortcutManager, [&shortcutManager] {
        shortcutManager.tryEnroll(true);
    });

    // Runtime style switch: SetLockScreenStyle() goes through the root helper,
    // which broadcasts LockScreenStyleChanged. Once the lock screen is hidden
    // the classic locker releases its names and execs the configured locker
    // in daemon mode.
    gxde::lock::LockStylePaths stylePaths =
        gxde::lock::LockStyleResolver::DefaultPaths();
    stylePaths.configFile = qEnvironmentVariable(
        "GXDE_LOCK_CONF", stylePaths.configFile);
    stylePaths.omgConfig = qEnvironmentVariable(
        "GXDE_LOCK_OMG_CONF", stylePaths.omgConfig);
    gxde::lock::LockStyleManager styleManager(
        model, gxde::lock::LockStyle::kClassic, stylePaths);
    QObject::connect(&displayManagerService,
                     &GxdeLockService::LockScreenStyleChanged,
                     &styleManager, &gxde::lock::LockStyleManager::Refresh);
    QObject::connect(
        &styleManager, &gxde::lock::LockStyleManager::HandoffRequested,
        &app, [&](const QString &executable) {
            qInfo() << "Lock screen style changed; handing off to"
                    << executable;
            const QStringList names{
                kGxdeLockService,
                DBUS_NAME,
                QStringLiteral("org.freedesktop.ScreenSaver"),
            };
            for (const QString &name : names) {
                conn.unregisterService(name);
            }
            gxde::lock::ExecLockScreen(executable, {QByteArrayLiteral("--daemon")});

            // exec failed: take the names back and keep running.
            conn.registerService(kGxdeLockService);
            conn.registerService(DBUS_NAME);
            conn.registerService(QStringLiteral("org.freedesktop.ScreenSaver"));
        },
        Qt::QueuedConnection);

    if (!runDaemon) {
        if (showUserList) {
            emit model->showUserList();
        } else {
            model->setIsShow(true);
        }
    }

    app.exec();

    return 0;
}
