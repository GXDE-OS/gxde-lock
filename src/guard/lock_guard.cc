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

#include "lock_guard.h"

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string.h>

#include <utility>
#include <vector>

#include <QByteArray>
#include <QDebug>

#include "LockState.h"

namespace gxde::lock {

namespace {

constexpr int kCrashLimit = 3;
constexpr qint64 kCrashWindowMs = 60 * 1000;
constexpr int kNoticeAttempts = 3;

volatile sig_atomic_t g_stop_signal = 0;
volatile pid_t g_child = 0;

void OnStopSignal(int signal) {
  g_stop_signal = signal;
  const pid_t child = g_child;
  if (child > 0) kill(child, signal);
}

void InstallStopHandlers() {
  struct sigaction action {};
  action.sa_handler = OnStopSignal;
  sigemptyset(&action.sa_mask);
  for (int signal : {SIGTERM, SIGINT, SIGHUP}) sigaction(signal, &action, nullptr);
}

QString DescribeStatus(int status) {
  if (WIFSIGNALED(status)) {
    return QStringLiteral("killed by signal %1 (%2)")
        .arg(WTERMSIG(status))
        .arg(QString::fromLocal8Bit(strsignal(WTERMSIG(status))));
  }
  return QStringLiteral("exited with status %1").arg(WEXITSTATUS(status));
}

}  // namespace

LockGuard::LockGuard(QString locker, QStringList args, QString notice)
    : locker_(std::move(locker)),
      args_(std::move(args)),
      notice_(std::move(notice)) {
  clock_.start();
}

QStringList LockGuard::CommandFor(Mode mode) const {
  if (mode == Mode::kNotice) return {notice_, QStringLiteral("--notice")};

  QStringList command{locker_};
  for (const QString& arg : args_) {
    if (mode != Mode::kAsRequested &&
        (arg == QLatin1String("--daemon") || arg == QLatin1String("-d"))) {
      continue;
    }
    command << arg;
  }
  // Without --daemon the locker locks as soon as it starts.
  if (mode == Mode::kDaemon) command << QStringLiteral("--daemon");
  return command;
}

pid_t LockGuard::Spawn(const QStringList& command) {
  std::vector<QByteArray> storage;
  storage.reserve(command.size());
  for (const QString& part : command) storage.push_back(part.toLocal8Bit());
  std::vector<char*> argv;
  for (QByteArray& part : storage) argv.push_back(part.data());
  argv.push_back(nullptr);

  const pid_t pid = fork();
  if (pid == 0) {
    execv(argv[0], argv.data());
    _exit(127);
  }
  if (pid < 0) {
    qCritical() << "(Guard) Cannot start" << command.first() << ":"
                << strerror(errno);
  }
  return pid;
}

int LockGuard::WaitFor(pid_t pid) {
  g_child = pid;
  // A stop signal that arrived before g_child was set has not been
  // forwarded yet.
  if (g_stop_signal) kill(pid, g_stop_signal);

  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) {
      status = 0;
      break;
    }
  }
  g_child = 0;
  return status;
}

bool LockGuard::CrashLoop() {
  const qint64 now = clock_.elapsed();
  crashes_ms_.append(now);
  while (!crashes_ms_.isEmpty() && now - crashes_ms_.first() > kCrashWindowMs)
    crashes_ms_.removeFirst();
  return crashes_ms_.size() >= kCrashLimit;
}

int LockGuard::Run() {
  InstallStopHandlers();
  // Anything left over belongs to a locker from an earlier session.
  MarkSessionLocked(false);

  Mode mode = Mode::kAsRequested;
  int notice_failures = 0;
  while (!g_stop_signal) {
    const QStringList command = CommandFor(mode);
    const pid_t pid = Spawn(command);
    if (pid < 0) return 1;
    const int status = WaitFor(pid);
    if (g_stop_signal) break;

    const bool clean = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (mode == Mode::kNotice) {
      if (clean) {
        // The notice only exits cleanly after the session was unlocked.
        qInfo() << "(Guard) Session unlocked from the crash notice;"
                   " restarting the locker.";
        MarkSessionLocked(false);
        crashes_ms_.clear();
        notice_failures = 0;
        mode = Mode::kDaemon;
        continue;
      }
      qCritical() << "(Guard) Crash notice" << DescribeStatus(status);
      if (++notice_failures >= kNoticeAttempts) {
        qCritical() << "(Guard) Giving up; the session cannot be guarded.";
        return 1;
      }
      continue;
    }

    // Exit status 0: another instance owns the session, or the locker was
    // asked to quit. Either way there is nothing left to guard.
    if (clean) return 0;

    const bool locked = IsSessionMarkedLocked();
    qWarning() << "(Guard) Locker" << DescribeStatus(status)
               << (locked ? "while the screen was locked" : "while idle");
    if (CrashLoop()) {
      if (!locked) {
        qCritical() << "(Guard) Locker keeps crashing; giving up.";
        return 1;
      }
      qCritical() << "(Guard) Locker keeps crashing while locked;"
                     " holding the lock with the crash notice.";
      mode = Mode::kNotice;
      continue;
    }
    mode = locked ? Mode::kLocked : Mode::kDaemon;
  }
  return 0;
}

}  // namespace gxde::lock
