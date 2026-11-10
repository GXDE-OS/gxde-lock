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

#ifndef SRC_CORE_EVENT_LOOP_EVENT_LOOP_H_
#define SRC_CORE_EVENT_LOOP_EVENT_LOOP_H_

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace gxde {
namespace lock {
namespace core {

/**
 * Evenr loop for unified event handling that supports fds, timers, and signls.
 * All events are processed in a single thread using epoll_wait().
 */
class EventLoop {
 public:
  /**
   * @brief The default constructor of @c EventLoop.
   */
  explicit EventLoop() = default;

  /**
   * @brief The destructor of a @c EventLoop, which is also responsible for
   *        closing fds.
   */
  ~EventLoop();

  // Disable copy & move.
  EventLoop(const EventLoop&) = delete;
  EventLoop& operator=(const EventLoop&) = delete;
  EventLoop(EventLoop&&) = delete;
  EventLoop& operator=(EventLoop&&) = delete;

  /**
   * @brief Initialize the event loop.
   * @return @c true if succeed, @c false on failure.
   */
  bool Initialize();

  /**
   * @brief Runs the main event loop.
   * @note It blocks until @c Quit() is called.
   */
  void Run();

  /**
   * @brief Request the event loop to quit.
   */
  void Quit();

  /**
   * @brief Check the running status of current event loop.
   * @return @c true if running and @c false otherwise.
   */
  bool IsRunning() const { return is_running_; }

  /**
   * @brief Adds a file descriptor to event loop.
   * 
   * @param fd       The file descriptor to add and monitor.
   * @param events   The event monitor to use.
   *                 e.g. @c EPOLLIN, @c EPOLLOUT, etc.
   * @param callback Function to call when event occurred.
   * @return @c true on succeed and @c false otherwise.
   */
  bool AddFd(int fd, uint32_t events, std::function<void(uint32_t)> callback);

  /**
   * @brief Modify the events for a file descriptor.
   * 
   * @param fd     The file descriptor to modify.
   * @param events New events that we may monitor.
   * @return @c true on succeed and @c false otherwise.
   */
  bool ModifyFd(int fd, uint32_t events);

  /**
   * @brief Removes a file descriptor from the event loop.
   * 
   * @param fd The file descriptor to remove.
   * @return @c true on succeed and @c false otherwise.
   */
  bool RemoveFd(int fd);

  /**
   * @brief This is a time id type.
   */
  using TimerId = uint64_t;

  /**
   * @brief Adds a timer to event loop.
   * 
   * @param interval_ms Interval in ms that the timer repeat.
   * @param callback    Function to call on a timer timeout.
   * @note You may set @c interval_ms to @c 0 for one-shot timer.
   * @return The ID of the timer.
   */
  uint64_t AddTimer(uint64_t interval_ms, std::function<void()> callback);

  /**
   * @brief Cancels a timer.
   * 
   * @param id The id of the timer to cancel.
   * @note The ID may be returned by @c AddTimer, so you need to save them
   *       after the @c AddTimer call to use them here.
   * @return @c true if the timer is found and cancelled, @c false otherwise.
   */
  bool CancelTimer(uint64_t id);

  /**
   * @brief Adds a signal handler.
   * 
   * @param signal_number The signal number. e.g. @c SIGINT, @c SIGTERM, etc.
   * @param callback      Function to call when the signal is received.
   * @return @c true if the timer is found and cancelled, @c false otherwise.
   */
  bool AddSignal(int signal_number, std::function<void()> callback);

 private:
  /**
   * @brief Data associated with file descriptor.
   */
  struct FdData {
    uint32_t events;                         ///< Monitored events.
    std::function<void(uint32_t)> callback;  ///< Event callback.
  };

  /**
   * @brief Data associated with timer.
   */
  struct TimerData {
    uint64_t interval_ms;            ///< Interval in milliseconds.
    uint64_t next_trigger_ms;        ///< Next trigger time in milliseconds.
    std::function<void()> callback;  ///< Timer callback.
  };

  int epoll_fd_ = -1;            ///< epoll file descriptor.
  int signal_fd_ = -1;           ///< signalfd file descriptor.
  bool is_running_ = false;      ///< Whether the loop is running.
  bool quit_requested_ = false;  ///< Whether quit was requested.

  std::unordered_map<int, FdData> fd_callbacks_;    ///< FD callbacks.
  std::unordered_map<uint64_t, TimerData> timers_;  ///< Active timers.
  uint64_t next_timer_id_ = 1;                      ///< Next timer ID.

  /**
   * @brief Processes pending signal events.
   */
  void ProcessSignalEvents();

  /**
   * @brief Processes expired events.
   */
  void ProcessTimerEvents();

  /**
   * @brief Gets current time in ms.
   * @return Current time (since epoch, in ms).
   */
  static uint64_t GetCurrentTimeMs();

  /**
   * @brief Calculates timeout for @c epoll_wait().
   * @return Timeout in ms, or @c -1 for infinite.
   */
  int CalculateTimeoutMs() const;
};

}  // namespace core
}  // namespace lock
}  // namespace gxde

#endif  // SRC_CORE_EVENT_LOOP_EVENT_LOOP_H_
