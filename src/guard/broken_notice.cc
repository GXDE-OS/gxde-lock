/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "broken_notice.h"

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <wayland-client.h>

#include <memory>

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QSocketNotifier>
#include <QStringList>
#include <QTimer>

#include "notice_window.h"
#include "waylandsessionlock.h"

// Last: Xlib's macros (None, KeyPress, Status, ...) break Qt headers.
#include <X11/Xlib.h>

namespace gxde::lock {

namespace {

constexpr int kGrabRetryMs = 200;
constexpr int kGrabAttempts = 50;

int g_unlock_pipe[2] = {-1, -1};

void OnUnlockSignal(int) {
  const char byte = 1;
  [[maybe_unused]] const ssize_t written = write(g_unlock_pipe[1], &byte, 1);
}

void RoundtripWayland() {
  auto* native =
      qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
  if (native && native->display()) wl_display_roundtrip(native->display());
}

Display* X11Display() {
  auto* native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
  return native ? native->display() : nullptr;
}

}  // namespace

BrokenNotice::BrokenNotice(QObject* parent)
    : QObject(parent),
      use_x11_(QGuiApplication::platformName().startsWith(
          QLatin1String("xcb"))) {
  if (pipe2(g_unlock_pipe, O_CLOEXEC | O_NONBLOCK) == 0) {
    unlock_signal_ = new QSocketNotifier(g_unlock_pipe[0],
                                         QSocketNotifier::Read, this);
    connect(unlock_signal_, &QSocketNotifier::activated, this, [this] {
      char byte;
      while (read(g_unlock_pipe[0], &byte, 1) > 0) {
      }
      qInfo() << "(Notice) Unlock requested by SIGUSR1.";
      Finish();
    });
    struct sigaction action {};
    action.sa_handler = OnUnlockSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &action, nullptr);
  }
  connect(&logind_, &LogindSession::UnlockRequested, this, [this] {
    qInfo() << "(Notice) Unlock requested through logind.";
    Finish();
  });
}

BrokenNotice::~BrokenNotice() {
  qDeleteAll(windows_);
  windows_.clear();
}

bool BrokenNotice::Start() {
  if (!use_x11_) {
    session_lock_ = std::make_unique<WaylandSessionLockIntegration>();
    session_lock_->initialize(nullptr);
    RoundtripWayland();
    if (!session_lock_->isAvailable()) {
      qWarning() << "(Notice) ext-session-lock-v1 is unavailable; the crashed"
                    " locker's surfaces are gone, nothing to hold.";
      return false;
    }
    connect(session_lock_.get(), &WaylandSessionLockIntegration::finished,
            this, [] {
              qCritical() << "(Notice) The compositor refused to hand over"
                             " the session lock.";
              QCoreApplication::exit(2);
            });
    connect(session_lock_.get(), &WaylandSessionLockIntegration::unlocked,
            this, [] {
              RoundtripWayland();
              QCoreApplication::quit();
            });
    // Take the abandoned lock over even while no output is connected.
    session_lock_->ensureLock();
  }

  for (QScreen* screen : QGuiApplication::screens()) AddScreen(screen);
  connect(qGuiApp, &QGuiApplication::screenAdded, this,
          &BrokenNotice::AddScreen);
  connect(qGuiApp, &QGuiApplication::screenRemoved, this,
          &BrokenNotice::RemoveScreen);
  if (use_x11_) GrabX11Input();

  logind_.SetLockedHint(true);
  qWarning().noquote() << "(Notice) Holding the lock:\n" << Message();
  return true;
}

QString BrokenNotice::Message() const {
  const uint vt = logind_.VtNumber();
  const QString console =
      QStringLiteral("Ctrl+Alt+F%1").arg(vt == 2 ? 3 : 2);
  const QString kill_command =
      QStringLiteral("kill -USR1 %1").arg(QCoreApplication::applicationPid());

  QStringList lines;
  lines << tr("GXDE Lock crashed repeatedly and cannot unlock the system.");
  //: %1 is a key combination such as Ctrl+Alt+F3.
  lines << tr("Press %1 to switch to a TTY, log in and run:").arg(console);
  if (logind_.IsValid()) {
    lines << QStringLiteral("loginctl unlock-session %1").arg(logind_.Id());
    //: An alternative command; %1 is "kill -USR1 <pid>".
    lines << tr("or: %1").arg(kill_command);
  } else {
    lines << kill_command;
  }
  if (vt > 0) {
    //: %1 is the number of this session's virtual terminal (Ctrl+Alt+F%1).
    lines << tr("Then return with Ctrl+Alt+F%1.").arg(vt);
  } else {
    lines << tr("Then switch back to this session.");
  }
  return lines.join(QStringLiteral("\n\n"));
}

void BrokenNotice::AddScreen(QScreen* screen) {
  if (!screen || windows_.contains(screen) || finishing_) return;

  auto* window = new NoticeWindow(Message());
  window->setScreen(screen);
  // Qt moves a window to whichever screen contains its position; keep each
  // one on its own output (see the lock screens).
  window->setGeometry(screen->geometry());
  if (use_x11_) {
    window->setFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint |
                     Qt::WindowStaysOnTopHint);
  } else if (!session_lock_->installOnWindow(window, screen)) {
    delete window;
    return;
  }
  windows_.insert(screen, window);
  window->show();
}

void BrokenNotice::RemoveScreen(QScreen* screen) {
  delete windows_.take(screen);
  if (use_x11_) GrabX11Input();
}

void BrokenNotice::GrabX11Input() {
  Display* display = X11Display();
  if (!display || windows_.isEmpty() || finishing_) return;

  const Window window = static_cast<Window>(windows_.cbegin().value()->winId());
  const int keyboard = XGrabKeyboard(display, window, True, GrabModeAsync,
                                     GrabModeAsync, CurrentTime);
  const int pointer = XGrabPointer(display, window, True, 0, GrabModeAsync,
                                   GrabModeAsync, None, None, CurrentTime);
  XFlush(display);
  if (keyboard == GrabSuccess && pointer == GrabSuccess) {
    grab_attempts_ = 0;
    return;
  }
  if (++grab_attempts_ < kGrabAttempts) {
    QTimer::singleShot(kGrabRetryMs, this, &BrokenNotice::GrabX11Input);
  } else {
    qWarning() << "(Notice) Could not grab the keyboard and pointer.";
  }
}

void BrokenNotice::Finish() {
  if (finishing_) return;
  finishing_ = true;
  logind_.SetLockedHint(false);

  if (session_lock_) {
    // unlock() returns false until the compositor confirmed the lock; the
    // unlocked() handler finishes the job then.
    if (session_lock_->unlock()) {
      RoundtripWayland();
      QCoreApplication::quit();
    }
    return;
  }

  if (Display* display = X11Display()) {
    XUngrabKeyboard(display, CurrentTime);
    XUngrabPointer(display, CurrentTime);
    XFlush(display);
  }
  QCoreApplication::quit();
}

}  // namespace gxde::lock
