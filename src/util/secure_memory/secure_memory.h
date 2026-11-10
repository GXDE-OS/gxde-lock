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

#ifndef SRC_UTIL_SECURE_MEMORY_SECURE_MEMORY_H_
#define SRC_UTIL_SECURE_MEMORY_SECURE_MEMORY_H_

#include <cstddef>
#include <string>

namespace gxde {
namespace lock {
namespace util {

/**
 * @brief Locks a memory region so that it is never written to swap.
 *
 * On failure the region simply stays unlocked, which keeps the caller
 * functional on systems with a restrictive @c RLIMIT_MEMLOCK.
 *
 * @param ptr  Pointer to the memory region to lock.
 * @param size Size of the region in bytes.
 * @return @c true on success, @c false otherwise.
 */
bool LockMemory(void* ptr, std::size_t size);

/**
 * @brief Releases a lock previously taken by @c LockMemory().
 * @param ptr  Pointer to the memory region to unlock.
 * @param size Size of the region in bytes.
 */
void UnlockMemory(void* ptr, std::size_t size);

/**
 * @brief Zeroes a memory region in a way the compiler cannot elide.
 * @param ptr  Pointer to the memory region.
 * @param size Size of the region in bytes.
 */
void SecureClear(void* ptr, std::size_t size);

/**
 * @brief Zeroes a @c std::string and clears it.
 * @param str The string to wipe.
 */
void SecureClear(std::string& str);

/**
 * @brief Allocates locked memory for @p size bytes.
 *
 * The returned block is guaranteed to be inaccessible to swap and core
 * dumps. It must be released with @c SecureFree().
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to the block, or @c nullptr on failure.
 */
void* SecureAlloc(std::size_t size);

/**
 * @brief Zeroes, unlocks and releases a block from @c SecureAlloc().
 * @param ptr  Pointer to the block to release; may be @c nullptr.
 * @param size Number of bytes that were requested from @c SecureAlloc().
 */
void SecureFree(void* ptr, std::size_t size);

}  // namespace util
}  // namespace lock
}  // namespace gxde

#endif  // SRC_UTIL_SECURE_MEMORY_SECURE_MEMORY_H_
