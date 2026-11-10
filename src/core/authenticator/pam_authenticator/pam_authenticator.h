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

#ifndef SRC_CORE_AUTHENTICATOR_PAM_AUTHENTICATOR_PAM_AUTHENTICATOR_H_
#define SRC_CORE_AUTHENTICATOR_PAM_AUTHENTICATOR_PAM_AUTHENTICATOR_H_

#include "core/authenticator/authenticator.h"

namespace gxde {
namespace lock {
namespace core {

/**
 * @brief PAM-based implementation of @c Authenticator.
 *
 * Authenticates users through the PAM stack configured for the requested
 * service name. The password is copied into locked memory for the duration of
 * the conversation and wiped before the call returns, so it never reaches
 * swap or a core dump.
 *
 * @note Authentication is synchronous, therefore @c Cancel() can only reset
 *       the reporting state and cannot abort an in-flight PAM exchange.
 */
class PamAuthenticator : public Authenticator {
 public:
  /**
   * @brief The default constructor of @c PamAuthenticator.
   */
  PamAuthenticator() = default;

  /**
   * @brief The destructor of @c PamAuthenticator.
   */
  ~PamAuthenticator() override = default;

  AuthResult Authenticate(const AuthRequest& request) override;

  void Cancel() override;

  bool IsAuthenticating() const override { return is_authenticating_; }

 private:
  bool is_authenticating_ = false;  ///< Whether authentication is in progress.
};

}  // namespace core
}  // namespace lock
}  // namespace gxde

#endif  // SRC_CORE_AUTHENTICATOR_PAM_AUTHENTICATOR_PAM_AUTHENTICATOR_H_
