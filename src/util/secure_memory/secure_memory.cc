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

#include "util/secure_memory/secure_memory.h"

#include <sys/mman.h>

#include <cstdlib>
#include <cstring>
#include <string>

#include "absl/log/log.h"

namespace gxde {
namespace lock {
namespace util {

bool LockMemory(void* ptr, std::size_t size) {
  if (!ptr || size == 0) {
    return true;
  }

  if (mlock(ptr, size) != 0) {
    LOG(WARNING) << "Failed to lock " << size
                 << " bytes of memory, secrets may reach swap";
    return false;
  }

  return true;
}

void UnlockMemory(void* ptr, std::size_t size) {
  if (!ptr || size == 0) {
    return;
  }

  if (munlock(ptr, size) != 0) {
    LOG(WARNING) << "Failed to unlock " << size << " bytes of memory";
  }
}

void SecureClear(void* ptr, std::size_t size) {
  if (!ptr || size == 0) {
    return;
  }

  // explicit_bzero is not elided by the optimizer, unlike memset.
  explicit_bzero(ptr, size);
}

void SecureClear(std::string& str) {
  if (str.empty()) {
    return;
  }

  explicit_bzero(str.data(), str.size());
  str.clear();
}

void* SecureAlloc(std::size_t size) {
  if (size == 0) {
    return nullptr;
  }

  void* ptr = std::malloc(size);
  if (!ptr) {
    return nullptr;
  }

  // Locking is best effort: a restrictive RLIMIT_MEMLOCK must not make the
  // whole daemon unusable, it only degrades the guarantee.
  LockMemory(ptr, size);

  return ptr;
}

void SecureFree(void* ptr, std::size_t size) {
  if (!ptr) {
    return;
  }

  // Wipe the contents before handing the pages back to the allocator.
  SecureClear(ptr, size);

  // Remove the page lock, if one was taken.
  UnlockMemory(ptr, size);

  std::free(ptr);
}

}  // namespace util
}  // namespace lock
}  // namespace gxde
