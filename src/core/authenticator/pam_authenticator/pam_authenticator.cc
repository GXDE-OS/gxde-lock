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

#include <security/pam_appl.h>

#include <cstdlib>
#include <cstring>
#include <memory>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "util/scope_guard/scope_guard.h"
#include "util/secure_memory/secure_allocator.h"
#include "util/secure_memory/secure_memory.h"

#include "core/authenticator/pam_authenticator/pam_authenticator.h"

namespace gxde {
namespace lock {
namespace core {
namespace {

/// Default lockout duration reported to the UI on @c PAM_MAXTRIES.
constexpr uint32_t kDefaultLockoutMinutes = 10;

/**
 * @brief Data passed to the PAM conversation handler.
 */
struct PamConversationData {
  util::SecureString password;  ///< Password to provide to PAM.
  bool is_cancelled = false;    ///< Whether the authentication was cancelled.
};

/**
 * @brief Releases a response array allocated by @c PamConversation().
 *
 * Every response string is wiped before being freed so that a password can
 * never survive in freed heap memory.
 *
 * @param responses The response array to release; may be @c nullptr.
 * @param count     Number of entries that were allocated.
 */
void FreeResponses(pam_response* responses, int count) {
  if (!responses) {
    return;
  }

  for (int i = 0; i < count; ++i) {
    if (responses[i].resp) {
      util::SecureClear(responses[i].resp, strlen(responses[i].resp));
      free(responses[i].resp);
    }
  }

  free(responses);
}

/**
 * @brief PAM conversation handler.
 *
 * Handles PAM messages and provides responses. Only password prompts are
 * answered; informational and error messages are logged and ignored.
 *
 * @param num_msg  Number of messages in @p msg.
 * @param msg      Array of PAM messages.
 * @param response Output array of PAM responses to fill.
 * @param data_ptr Pointer to the @c PamConversationData instance.
 * @return @c PAM_SUCCESS on success, or a PAM error code otherwise.
 */
int PamConversation(int num_msg, const pam_message** msg,
                    pam_response** response, void* data_ptr) {
  if (num_msg <= 0 || num_msg > PAM_MAX_NUM_MSG) {
    return PAM_CONV_ERR;
  }

  auto* data = static_cast<PamConversationData*>(data_ptr);
  if (!data || data->is_cancelled) {
    return PAM_CONV_ERR;
  }

  // Allocate the response array.
  auto* responses = static_cast<pam_response*>(
      calloc(static_cast<size_t>(num_msg), sizeof(pam_response)));
  if (!responses) {
    return PAM_BUF_ERR;
  }

  // Release everything allocated so far on failure.
  const auto abort_with = [responses, num_msg](int code) {
    FreeResponses(responses, num_msg);
    return code;
  };

  for (int i = 0; i < num_msg; ++i) {
    switch (msg[i]->msg_style) {
      case PAM_PROMPT_ECHO_OFF:
      case PAM_PROMPT_ECHO_ON:
        // Password prompt.
        responses[i].resp = strdup(data->password.c_str());
        if (!responses[i].resp) {
          return abort_with(PAM_BUF_ERR);
        }

        responses[i].resp_retcode = 0;
        break;

      case PAM_ERROR_MSG:
        LOG(WARNING) << "PAM error: " << msg[i]->msg;
        responses[i].resp = nullptr;
        responses[i].resp_retcode = 0;
        break;

      case PAM_TEXT_INFO:
        LOG(INFO) << "PAM info: " << msg[i]->msg;
        responses[i].resp = nullptr;
        responses[i].resp_retcode = 0;
        break;

      default:
        // Unknown message type, abort the conversation.
        return abort_with(PAM_CONV_ERR);
    }
  }

  *response = responses;
  return PAM_SUCCESS;
}

}  // namespace

AuthResult PamAuthenticator::Authenticate(const AuthRequest& request) {
  AuthResult result;
  is_authenticating_ = true;

  // Always reset the flag, however we leave this scope.
  GXDE_SCOPE_GUARD({
    is_authenticating_ = false;
  });

  PamConversationData conversation_data;
  conversation_data.password = request.password;

  pam_conv conv = {};
  conv.conv = PamConversation;
  conv.appdata_ptr = &conversation_data;

  pam_handle_t* pam_handle = nullptr;
  int ret = pam_start(request.service.c_str(), request.username.c_str(), &conv,
                      &pam_handle);
  if (ret != PAM_SUCCESS) {
    result.error_message = "Failed to start PAM session";
    LOG(ERROR) << "pam_start failed: " << pam_strerror(nullptr, ret);

    util::SecureClear(conversation_data.password);
    return result;
  }

  // Always end the transaction and wipe the password copy.
  GXDE_SCOPE_GUARD({
    pam_end(pam_handle, PAM_SUCCESS);
    util::SecureClear(conversation_data.password);
  });

  // Perform the authentication.
  ret = pam_authenticate(pam_handle, 0);
  if (ret == PAM_SUCCESS) {
    // Verify that the account is still valid.
    ret = pam_acct_mgmt(pam_handle, 0);
  }

  if (ret == PAM_SUCCESS) {
    result.success = true;
    LOG(INFO) << "PAM authentication succeeded";
    return result;
  }

  result.error_message = pam_strerror(pam_handle, ret);

  // Report the lockout so the UI can show a countdown.
  if (ret == PAM_MAXTRIES) {
    result.locked_out = true;
    // TODO(CharOfString): Query the real remaining lockout time from PAM.
    result.lockout_minutes = kDefaultLockoutMinutes;
  }

  LOG(WARNING) << "PAM authentication failed: " << result.error_message;
  return result;
}

void PamAuthenticator::Cancel() {
  // PAM authentication is synchronous and cannot be truly cancelled, so only
  // the reporting state can be reset here.
  is_authenticating_ = false;
}

std::unique_ptr<Authenticator> CreatePamAuthenticator() {
  return std::make_unique<PamAuthenticator>();
}

}  // namespace core
}  // namespace lock
}  // namespace gxde
