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

#include <absl/log/check.h>
#include <absl/log/log.h>

#include <string>

#include "core/session_manager/session_manager.h"

namespace gxde {
namespace lock {
namespace core {

SessionManager::SessionManager() = default;

SessionManager::~SessionManager() {
  Shutdown();
}

bool SessionManager::Initialize() {
  authenticator_ = CreatePamAuthenticator();
  if (!authenticator_) {
    LOG(ERROR) << "Failed to create PAM authenticator";
    return false;
  }

  last_activity_ = std::chrono::steady_clock::now();
  LOG(INFO) << "Session manager initialized";
  return true;
}

void SessionManager::Shutdown() {
  if (authenticator_) {
    authenticator_->Cancel();
    authenticator_.reset();
  }
}

void SessionManager::RequestLock() {
  if (state_ == SessionState::kLocked || state_ == SessionState::kLocking) {
    LOG(INFO) << "Lock already requested or in progress";
    return;
  }

  LOG(INFO) << "Lock requested";
  SetState(SessionState::kLocking);

  if (lock_request_callback_) {
    lock_request_callback_();
  }
}

void SessionManager::RequestUnlock() {
  if (state_ != SessionState::kLocked) {
    LOG(INFO) << "Not locked, ignoring unlock request";
    return;
  }

  LOG(INFO) << "Unlock requested";
  SetState(SessionState::kAuthenticating);

  if (unlock_request_callback_) {
    unlock_request_callback_();
  }
}

void SessionManager::Authenticate(const std::string& password) {
  if (state_ != SessionState::kAuthenticating) {
    LOG(WARNING) << "Not in authenticating state";
    return;
  }

  if (!authenticator_) {
    LOG(ERROR) << "No authenticator available";
    return;
  }

  AuthRequest request;
  request.username = username_;
  request.password = password;
  request.service = "gxde-lock";

  // Note: PAM authentication may block.
  // TODO(CharOfString): Consider using asynchronous authentication.
  AuthResult result = authenticator_->Authenticate(request);
  OnAuthenticationComplete(result);
}

void SessionManager::CancelAuthentication() {
  if (authenticator_ && authenticator_->IsAuthenticating()) {
    authenticator_->Cancel();
  }

  if (state_ == SessionState::kAuthenticating) {
    SetState(SessionState::kLocked);
  }
}

void SessionManager::SetIdleTimeout(std::chrono::seconds timeout) {
  idle_timeout_ = timeout;
  LOG(INFO) << "Idle timeout set to " << timeout.count() << " seconds";
}

void SessionManager::OnUserActivity() {
  last_activity_ = std::chrono::steady_clock::now();
}

void SessionManager::SetCurrentUser(const std::string& username,
                                    const std::string& display_name,
                                    const std::string& avatar_path) {
  username_ = username;
  display_name_ = display_name;
  avatar_path_ = avatar_path;
  LOG(INFO) << "Current user set to: " << username_;
}

void SessionManager::SetState(SessionState new_state) {
  if (state_ == new_state) {
    return;
  }

  SessionState old_state = state_;
  state_ = new_state;

  LOG(INFO) << "State changed: " << static_cast<int>(old_state) << " -> "
            << static_cast<int>(new_state);

  if (state_change_callback_) {
    state_change_callback_(old_state, new_state);
  }
}

void SessionManager::OnAuthenticationComplete(const AuthResult& result) {
  if (result.success) {
    LOG(INFO) << "Authentication succeeded";
    SetState(SessionState::kUnlocking);

    if (auth_result_callback_) {
      auth_result_callback_(true, "");
    }
  } else {
    LOG(WARNING) << "Authentication failed: " << result.error_message;
    SetState(SessionState::kLocked);

    if (auth_result_callback_) {
      auth_result_callback_(false, result.error_message);
    }
  }
}

}  // namespace core
}  // namespace lock
}  // namespace gxde
