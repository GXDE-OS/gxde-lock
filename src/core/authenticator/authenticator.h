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

#ifndef SRC_CORE_AUTHENTICATOR_AUTHENTICATOR_H_
#define SRC_CORE_AUTHENTICATOR_AUTHENTICATOR_H_

#include <cstdint>
#include <memory>
#include <string>

namespace gxde {
namespace lock {
namespace core {

/**
 * @brief Authentication request data.
 */
struct AuthRequest {
  std::string username;               ///< Username to authenticate.
  std::string password;               ///< Password for authentication.
  std::string service = "gxde-lock";  ///< PAM service name.
};

/**
 * @brief Authentication result data.
 */
struct AuthResult {
  bool success = false;          ///< Whether authentication succeeded.
  std::string error_message;     ///< Error message on failure.
  bool locked_out = false;       ///< Whether the user is locked out.
  uint32_t lockout_minutes = 0;  ///< Remaining lockout minutes.
};

/**
 * @brief Abstract interface for authentication providers.
 *
 * Implementations must provide PAM-based or other authentication mechanisms.
 * They are allowed to block while the user is being verified.
 */
class Authenticator {
 public:
  virtual ~Authenticator() = default;

  // Disable copy and move semantics.
  Authenticator(const Authenticator&) = delete;
  Authenticator& operator=(const Authenticator&) = delete;
  Authenticator(Authenticator&&) = delete;
  Authenticator& operator=(Authenticator&&) = delete;

  /**
   * @brief Performs authentication.
   * @param request The authentication request data.
   * @return The result of the authentication.
   * @note This operation may block while waiting for the PAM response.
   */
  virtual AuthResult Authenticate(const AuthRequest& request) = 0;

  /**
   * @brief Cancels the ongoing authentication.
   */
  virtual void Cancel() = 0;

  /**
   * @brief Checks whether authentication is in progress.
   * @return @c true if authentication is in progress, @c false otherwise.
   */
  virtual bool IsAuthenticating() const = 0;

 protected:
  /**
   * @brief Default constructor of the authenticator interface.
   */
  Authenticator() = default;
};

/**
 * @brief Creates a PAM-based authenticator.
 * @return The unique pointer to the authenticator.
 */
std::unique_ptr<Authenticator> CreatePamAuthenticator();

}  // namespace core
}  // namespace lock
}  // namespace gxde

#endif  // SRC_CORE_AUTHENTICATOR_AUTHENTICATOR_H_
