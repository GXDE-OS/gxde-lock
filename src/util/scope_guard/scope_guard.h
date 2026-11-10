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

#ifndef SRC_UTIL_SCOPE_GUARD_SCOPE_GUARD_H_
#define SRC_UTIL_SCOPE_GUARD_SCOPE_GUARD_H_

#include <cstddef>
#include <type_traits>
#include <utility>

namespace gxde {
namespace lock {
namespace util {

/**
 * @brief A move-only type-erased callable invoked on scope exit.
 *
 * This is the storage layer used by @c ScopeGuard. It owns a heap allocated
 * invoker that knows how to destroy the concrete callable and how to run it.
 * The small-object optimization is deliberately not used so that the type
 * erased layout stays trivial across translation units.
 */
class ScopeGuardCallback {
 public:
  /**
   * @brief Constructs an empty callback that does nothing.
   */
  ScopeGuardCallback() noexcept;

  /**
   * @brief Destroys the callback, releasing the owned callable.
   */
  ~ScopeGuardCallback();

  /**
   * @brief Takes ownership of an already allocated invoker.
   * @param invoker Invoker returned by @c From; may be @c nullptr.
   * @param storage Heap block owned by the invoker, released on destruction.
   */
  ScopeGuardCallback(void (*invoke)(void*),
                     void (*destroy)(void*),
                     void* storage) noexcept;

  // The callback is a unique owner, so copying is not allowed.
  ScopeGuardCallback(const ScopeGuardCallback&) = delete;
  ScopeGuardCallback& operator=(const ScopeGuardCallback&) = delete;

  /**
   * @brief Move constructor, transfers ownership of the callable.
   * @param other The callback to move from.
   */
  ScopeGuardCallback(ScopeGuardCallback&& other) noexcept;

  /**
   * @brief Move assignment, transfers ownership of the callable.
   * @param other The callback to move from.
   * @return @c *this.
   */
  ScopeGuardCallback& operator=(ScopeGuardCallback&& other) noexcept;

  /**
   * @brief Builds a callback owning a copy of @p function.
   * @tparam F Type of the callable.
   * @param function The callable to store.
   * @return A callback that invokes @p function when executed.
   */
  template <typename F>
  static ScopeGuardCallback From(F&& function);

  /**
   * @brief Runs the stored callable.
   *
   * Behaves as a no-op when the callback is empty.
   */
  void Invoke();

  /**
   * @brief Drops the stored callable without invoking it.
   */
  void Reset() noexcept;

  /**
   * @brief Checks whether the callback still owns a callable.
   * @return @c true if a callable is owned, @c false otherwise.
   */
  bool IsValid() const noexcept { return storage_ != nullptr; }

 private:
  void (*invoke_)(void*);   ///< Type-erased invoker for the callable.
  void (*destroy_)(void*);  ///< Type-erased destructor for the callable.
  void* storage_;           ///< Heap block holding the callable.
};

/**
 * @brief Scope guard that executes a callable when leaving its scope.
 *
 * Similar to Go's @c defer or Rust's @c Drop. The callable is executed when
 * the @c ScopeGuard is destroyed, unless @c Dismiss() or @c Execute() has been
 * called first.
 */
class ScopeGuard {
 public:
  /**
   * @brief Constructs a scope guard that owns the given callback.
   * @param callback The callback to execute on destruction.
   */
  explicit ScopeGuard(ScopeGuardCallback callback) noexcept;

  /**
   * @brief Constructs a scope guard from any callable.
   * @tparam F Type of the callable.
   * @param function Callable to execute on destruction.
   */
  template <typename F>
  explicit ScopeGuard(F&& function)
      : ScopeGuard(ScopeGuardCallback::From(std::forward<F>(function))) {}

  /**
   * @brief Destructor, executes the callable if the guard is still active.
   */
  ~ScopeGuard();

  // The guard is a unique owner, so copying is not allowed.
  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;

  /**
   * @brief Move constructor, transfers ownership of the callback.
   * @param other The guard to move from.
   */
  ScopeGuard(ScopeGuard&& other) noexcept;

  /**
   * @brief Move assignment, runs the current callable and adopts @p other.
   * @param other The guard to move from.
   * @return @c *this.
   */
  ScopeGuard& operator=(ScopeGuard&& other) noexcept;

  /**
   * @brief Cancels the execution of the callable.
   */
  void Dismiss() noexcept;

  /**
   * @brief Executes the callable immediately.
   */
  void Execute();

 private:
  ScopeGuardCallback callback_;  ///< The callable owned by the guard.
  bool is_active_;               ///< Whether the callable still runs.
};

/**
 * @brief Creates a @c ScopeGuard for the given callable.
 * @tparam F Type of the callable.
 * @param function Callable to execute on scope exit.
 * @return The constructed @c ScopeGuard instance.
 */
template <typename F>
ScopeGuard MakeScopeGuard(F&& function) {
  return ScopeGuard(std::forward<F>(function));
}

template <typename F>
ScopeGuardCallback ScopeGuardCallback::From(F&& function) {
  using Callable = std::decay_t<F>;
  auto* storage = new Callable(std::forward<F>(function));

  return ScopeGuardCallback(
      [](void* data) { (*static_cast<Callable*>(data))(); },
      [](void* data) { delete static_cast<Callable*>(data); }, storage);
}

}  // namespace util
}  // namespace lock
}  // namespace gxde

// Helper macro to build unique identifiers.
#define GXDE_CONCAT_IMPL(x, y) x##y
#define GXDE_CONCAT(x, y) GXDE_CONCAT_IMPL(x, y)

/**
 * @brief Creates a scope guard with an automatically generated variable name.
 * @param code Statements to execute on scope exit.
 */
#define GXDE_SCOPE_GUARD(code)                \
  auto GXDE_CONCAT(_scope_guard_, __LINE__) = \
      ::gxde::lock::util::MakeScopeGuard([&]() { code; })

#endif  // SRC_UTIL_SCOPE_GUARD_SCOPE_GUARD_H_
