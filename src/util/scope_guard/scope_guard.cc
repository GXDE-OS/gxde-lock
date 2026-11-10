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

#include "util/scope_guard/scope_guard.h"

#include <utility>

namespace gxde {
namespace lock {
namespace util {

ScopeGuardCallback::ScopeGuardCallback() noexcept
    : invoke_(nullptr), destroy_(nullptr), storage_(nullptr) {}

ScopeGuardCallback::ScopeGuardCallback(void (*invoke)(void*),
                                       void (*destroy)(void*),
                                       void* storage) noexcept
    : invoke_(invoke), destroy_(destroy), storage_(storage) {}

ScopeGuardCallback::~ScopeGuardCallback() {
  Reset();
}

ScopeGuardCallback::ScopeGuardCallback(ScopeGuardCallback&& other) noexcept
    : invoke_(other.invoke_),
      destroy_(other.destroy_),
      storage_(other.storage_) {
  other.invoke_ = nullptr;
  other.destroy_ = nullptr;
  other.storage_ = nullptr;
}

ScopeGuardCallback& ScopeGuardCallback::operator=(
    ScopeGuardCallback&& other) noexcept {
  if (this != &other) {
    // Release the callable we currently own before adopting the new one.
    Reset();

    invoke_ = other.invoke_;
    destroy_ = other.destroy_;
    storage_ = other.storage_;

    other.invoke_ = nullptr;
    other.destroy_ = nullptr;
    other.storage_ = nullptr;
  }
  return *this;
}

void ScopeGuardCallback::Invoke() {
  if (invoke_ && storage_) {
    invoke_(storage_);
  }
}

void ScopeGuardCallback::Reset() noexcept {
  if (storage_) {
    destroy_(storage_);
  }

  invoke_ = nullptr;
  destroy_ = nullptr;
  storage_ = nullptr;
}

ScopeGuard::ScopeGuard(ScopeGuardCallback callback) noexcept
    : callback_(std::move(callback)), is_active_(true) {}

ScopeGuard::~ScopeGuard() {
  if (is_active_) {
    callback_.Invoke();
  }
}

ScopeGuard::ScopeGuard(ScopeGuard&& other) noexcept
    : callback_(std::move(other.callback_)), is_active_(other.is_active_) {
  other.is_active_ = false;
}

ScopeGuard& ScopeGuard::operator=(ScopeGuard&& other) noexcept {
  if (this != &other) {
    // Honour the pending do-not-forget semantics of the current guard.
    if (is_active_) {
      callback_.Invoke();
    }

    callback_ = std::move(other.callback_);
    is_active_ = other.is_active_;
    other.is_active_ = false;
  }
  return *this;
}

void ScopeGuard::Dismiss() noexcept {
  is_active_ = false;
}

void ScopeGuard::Execute() {
  if (is_active_) {
    callback_.Invoke();
    is_active_ = false;
  }
}

}  // namespace util
}  // namespace lock
}  // namespace gxde
