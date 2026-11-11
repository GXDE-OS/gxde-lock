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

#ifndef SRC_CORE_SESSION_MANAGER_SESSION_MANAGER_H_
#define SRC_CORE_SESSION_MANAGER_SESSION_MANAGER_H_

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "core/authenticator/authenticator.h"

namespace gxde {
namespace lock {
namespace core {

/**
 * @brief Session state machine states.
 */
enum class SessionState {
  kUnlocked,        ///< Screen is unlocked.
  kLocking,         ///< Lock is in progress (creating windows/surfaces).
  kLocked,          ///< Screen is locked.
  kUnlocking,       ///< Unlock is in progress (animation).
  kAuthenticating,  ///< Authentication is in progress.
};

/**
 * @brief Manages the lock screen session state machine.
 *
 * Coordinates between the platform backend, authentication, and D-Bus
 * service. Handles lock/unlock requests, authentication, and idle timeout.
 */
class SessionManager {
 public:
  /**
   * @brief The default constructor of @c SessionManager.
   */
  SessionManager();

  /**
   * @brief The destructor of @c SessionManager.
   */
  ~SessionManager();

  // Disable copy and move semantics.
  SessionManager(const SessionManager&) = delete;
  SessionManager& operator=(const SessionManager&) = delete;
  SessionManager(SessionManager&&) = delete;
  SessionManager& operator=(SessionManager&&) = delete;

  /**
   * @brief Initializes the session manager.
   * @return @c true on success, @c false on failure.
   */
  bool Initialize();

  /**
   * @brief Shuts down the session manager.
   */
  void Shutdown();

  /**
   * @brief Gets the current session state.
   * @return Current state.
   */
  SessionState GetState() const { return state_; }

  /**
   * @brief Checks if the screen is locked.
   * @return @c true if locked, @c false otherwise.
   */
  bool IsLocked() const { return state_ == SessionState::kLocked; }

  /**
   * @brief Checks if authentication is in progress.
   * @return @c true if authenticating, @c false otherwise.
   */
  bool IsAuthenticating() const {
    return state_ == SessionState::kAuthenticating;
  }

  /**
   * @brief Requests the screen to be locked.
   */
  void RequestLock();

  /**
   * @brief Requests the screen to be unlocked.
   */
  void RequestUnlock();

  /**
   * @brief Submits a password for authentication.
   * @param password The password to authenticate.
   */
  void Authenticate(const std::string& password);

  /**
   * @brief Cancels ongoing authentication.
   */
  void CancelAuthentication();

  /**
   * @brief Sets the idle timeout for automatic locking.
   * @param timeout Timeout duration.
   */
  void SetIdleTimeout(std::chrono::seconds timeout);

  /**
   * @brief Notifies the manager of user activity.
   */
  void OnUserActivity();

  /**
   * @brief Sets the current user information.
   * @param username Username.
   * @param display_name Display name.
   * @param avatar_path Path to avatar image.
   */
  void SetCurrentUser(const std::string& username,
                      const std::string& display_name,
                      const std::string& avatar_path);

  /**
   * @brief Callback type for state changes.
   */
  using StateChangeCallback =
      std::function<void(SessionState old_state, SessionState new_state)>;

  /**
   * @brief Sets the state change callback.
   * @param callback Function to call on state changes.
   */
  void SetStateChangeCallback(StateChangeCallback callback) {
    state_change_callback_ = std::move(callback);
  }

  /**
   * @brief Callback type for authentication results.
   */
  using AuthResultCallback =
      std::function<void(bool success, const std::string& message)>;

  /**
   * @brief Sets the authentication result callback.
   * @param callback Function to call when authentication completes.
   */
  void SetAuthResultCallback(AuthResultCallback callback) {
    auth_result_callback_ = std::move(callback);
  }

  /**
   * @brief Callback type for lock requests.
   */
  using LockRequestCallback = std::function<void()>;

  /**
   * @brief Sets the lock request callback.
   * @param callback Function to call when lock is requested.
   */
  void SetLockRequestCallback(LockRequestCallback callback) {
    lock_request_callback_ = std::move(callback);
  }

  /**
   * @brief Callback type for unlock requests.
   */
  using UnlockRequestCallback = std::function<void()>;

  /**
   * @brief Sets the unlock request callback.
   * @param callback Function to call when unlock is requested.
   */
  void SetUnlockRequestCallback(UnlockRequestCallback callback) {
    unlock_request_callback_ = std::move(callback);
  }

 private:
  SessionState state_ = SessionState::kUnlocked;  ///< Current session state.

  // User information
  std::string username_;        ///< Current username.
  std::string display_name_;    ///< Current user display name.
  std::string avatar_path_;     ///< Path to user avatar.

  std::unique_ptr<Authenticator> authenticator_;  ///< Authentication provider.

  // Idle detection
  std::chrono::seconds idle_timeout_{300};  ///< Idle timeout (default 5 min).
  std::chrono::steady_clock::time_point last_activity_;

  // Callbacks
  StateChangeCallback state_change_callback_;      ///< State change callback.
  AuthResultCallback auth_result_callback_;        ///< Auth result callback.
  LockRequestCallback lock_request_callback_;      ///< Lock callback.
  UnlockRequestCallback unlock_request_callback_;  ///< Unlock callback.

  /**
   * @brief Changes the session state.
   * @param new_state The new state to transition to.
   */
  void SetState(SessionState new_state);

  /**
   * @brief Handles authentication completion.
   * @param result The authentication result.
   */
  void OnAuthenticationComplete(const AuthResult& result);
};

}  // namespace core
}  // namespace lock
}  // namespace gxde

#endif  // SRC_CORE_SESSION_MANAGER_SESSION_MANAGER_H_
