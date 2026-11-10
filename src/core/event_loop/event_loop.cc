/*  Copyright (C) 2026 CharOfString <root@charofstring.cc>
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

#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "core/event_loop/event_loop.h"

namespace gxde {
namespace lock {
namespace core {
namespace {

constexpr int kMaxEvents = 64;

}  // namespace

EventLoop::~EventLoop() {
  if (epoll_fd_ >= 0) {
    close(epoll_fd_);
  }

  if (signal_fd_ >= 0) {
    close(signal_fd_);
  }
}

bool EventLoop::Initialize() {
  // Create epoll instance.
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0) {
    LOG(ERROR) << "Failed to create epoll: " << strerror(errno);
    return false;
  }

  // Set up signal mask and create signalfd.
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGQUIT);
  sigaddset(&mask, SIGHUP);

  // Block signals so signalfd handles them.
  if (sigprocmask(SIG_BLOCK, &mask, nullptr) < 0) {
    LOG(ERROR) << "Failed to block signals: " << strerror(errno);
    close(epoll_fd_);
    epoll_fd_ = -1;
    return false;
  }

  signal_fd_ = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
  if (signal_fd_ < 0) {
    LOG(ERROR) << "Failed to create signalfd: " << strerror(errno);
    close(epoll_fd_);
    epoll_fd_ = -1;
    return false;
  }

  // Add signalfd to epoll.
  struct epoll_event ev = {};
  ev.events = EPOLLIN;
  ev.data.fd = signal_fd_;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, signal_fd_, &ev) < 0) {
    LOG(ERROR) << "Failed to add signalfd to epoll: " << strerror(errno);
    close(signal_fd_);
    close(epoll_fd_);
    signal_fd_ = -1;
    epoll_fd_ = -1;
    return false;
  }

  LOG(INFO) << "Event loop initialized";
  return true;
}

void EventLoop::Run() {
  if (epoll_fd_ < 0) {
    LOG(ERROR) << "Event loop not initialized";
    return;
  }

  is_running_ = true;
  quit_requested_ = false;

  struct epoll_event events[kMaxEvents];

  while (!quit_requested_) {
    int timeout = CalculateTimeoutMs();
    int nfds = epoll_wait(epoll_fd_, events, kMaxEvents, timeout);

    if (nfds < 0) {
      if (errno == EINTR) {
        continue;
      }
      LOG(ERROR) << "epoll_wait failed: " << strerror(errno);
      break;
    }

    // Process file descriptor events.
    for (int i = 0; i < nfds; ++i) {
      int fd = events[i].data.fd;
      uint32_t event_flags = events[i].events;

      if (fd == signal_fd_) {
        ProcessSignalEvents();
        continue;
      }

      auto it = fd_callbacks_.find(fd);
      if (it != fd_callbacks_.end()) {
        it->second.callback(event_flags);
      }
    }

    // Process timer events.
    ProcessTimerEvents();
  }

  is_running_ = false;
  LOG(INFO) << "Event loop stopped";
}

void EventLoop::Quit() {
  quit_requested_ = true;
}

bool EventLoop::AddFd(int fd, uint32_t events,
                      std::function<void(uint32_t)> callback) {
  if (epoll_fd_ < 0) {
    return false;
  }

  struct epoll_event ev = {};
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
    LOG(ERROR) << "Failed to add fd " << fd << " to epoll: " << strerror(errno);
    return false;
  }

  fd_callbacks_[fd] = {events, std::move(callback)};
  return true;
}

bool EventLoop::ModifyFd(int fd, uint32_t events) {
  if (epoll_fd_ < 0) {
    return false;
  }

  auto it = fd_callbacks_.find(fd);
  if (it == fd_callbacks_.end()) {
    return false;
  }

  struct epoll_event ev = {};
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
    LOG(ERROR) << "Failed to modify fd " << fd << " in epoll: "
               << strerror(errno);
    return false;
  }

  it->second.events = events;
  return true;
}

bool EventLoop::RemoveFd(int fd) {
  if (epoll_fd_ < 0) {
    return false;
  }

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
    LOG(ERROR) << "Failed to remove fd " << fd << " from epoll: "
               << strerror(errno);
    return false;
  }

  fd_callbacks_.erase(fd);
  return true;
}

uint64_t EventLoop::AddTimer(uint64_t interval_ms,
                             std::function<void()> callback) {
  uint64_t id = next_timer_id_++;
  uint64_t now = GetCurrentTimeMs();

  TimerData data;
  data.interval_ms = interval_ms;
  data.next_trigger_ms = now + interval_ms;
  data.callback = std::move(callback);

  timers_[id] = std::move(data);
  return id;
}

bool EventLoop::CancelTimer(uint64_t id) {
  return timers_.erase(id) > 0;
}

bool EventLoop::AddSignal(int signum,
                          std::function<void()> callback [[maybe_unused]]) {
  LOG(WARNING) << "AddSignal(" << signum
               << ") not fully implemented, using default mask";
  return false;
}

void EventLoop::ProcessSignalEvents() {
  struct signalfd_siginfo fdsi;
  ssize_t s;

  while (true) {
    s = read(signal_fd_, &fdsi, sizeof(fdsi));
    if (s != sizeof(fdsi)) {
      break;
    }

    switch (fdsi.ssi_signo) {
      case SIGINT:
      case SIGTERM:
      case SIGQUIT:
        LOG(INFO) << "Received termination signal";
        Quit();
        break;
      case SIGHUP:
        LOG(INFO) << "Received SIGHUP";
        break;
      default:
        LOG(WARNING) << "Received unknown signal: " << fdsi.ssi_signo;
        break;
    }
  }
}

void EventLoop::ProcessTimerEvents() {
  uint64_t now = GetCurrentTimeMs();
  std::vector<uint64_t> expired;

  // Find all expired timers.
  for (const auto& [id, timer] : timers_) {
    if (timer.next_trigger_ms <= now) {
      expired.push_back(id);
    }
  }

  // Execute callbacks and update or remove timers.
  for (uint64_t id : expired) {
    auto it = timers_.find(id);
    if (it == timers_.end()) {
      continue;
    }

    it->second.callback();

    if (it->second.interval_ms > 0) {
      // Recurring timer: update next trigger time.
      it->second.next_trigger_ms = now + it->second.interval_ms;
    } else {
      // One-shot timer: remove.
      timers_.erase(it);
    }
  }
}

uint64_t EventLoop::GetCurrentTimeMs() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000 +
         static_cast<uint64_t>(ts.tv_nsec) / 1000000;
}

int EventLoop::CalculateTimeoutMs() const {
  if (timers_.empty()) {
    return -1;  // Wait indefinitely.
  }

  uint64_t now = GetCurrentTimeMs();
  uint64_t nearest = UINT64_MAX;

  for (const auto& [id, timer] : timers_) {
    if (timer.next_trigger_ms < nearest) {
      nearest = timer.next_trigger_ms;
    }
  }

  if (nearest <= now) {
    return 0;  // Trigger immediately.
  }

  uint64_t diff = nearest - now;
  return static_cast<int>(diff > INT32_MAX ? INT32_MAX : diff);
}

}  // namespace core
}  // namespace lock
}  // namespace gxde
