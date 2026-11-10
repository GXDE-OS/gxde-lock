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

#ifndef SRC_UTIL_SECURE_MEMORY_SECURE_ALLOCATOR_H_
#define SRC_UTIL_SECURE_MEMORY_SECURE_ALLOCATOR_H_

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "util/secure_memory/secure_memory.h"

namespace gxde {
namespace lock {
namespace util {

/**
 * @brief Allocator that keeps its storage locked in physical memory.
 *
 * Memory obtained from this allocator is locked with @c mlock() so that it can
 * never be written to swap or a core dump, and it is wiped before being
 * returned to the heap. This makes it suitable for holding passwords and
 * other short lived secrets inside standard containers.
 *
 * @tparam T Type of the allocated objects.
 */
template <typename T>
class SecureAllocator {
 public:
  /// @brief Value type required by the allocator concept.
  using value_type = T;

  /**
   * @brief The default constructor of @c SecureAllocator.
   */
  SecureAllocator() = default;

  /**
   * @brief Rebinding constructor required by the allocator concept.
   * @tparam U Type of the other allocator.
   */
  template <typename U>
  explicit SecureAllocator(const SecureAllocator<U>&) {}

  /**
   * @brief Allocates locked memory for @p n objects of type @c T.
   * @param n Number of objects to allocate.
   * @return Pointer to the allocated memory, or @c nullptr on failure.
   */
  T* allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
      return nullptr;
    }

    void* ptr = SecureAlloc(n * sizeof(T));
    if (!ptr) {
      return nullptr;
    }

    return static_cast<T*>(ptr);
  }

  /**
   * @brief Zeroes, unlocks and releases previously allocated memory.
   * @param ptr Pointer to the memory to release; may be @c nullptr.
   * @param n   Number of objects that were allocated.
   */
  void deallocate(T* ptr, std::size_t n) {
    SecureFree(ptr, n * sizeof(T));
  }
};

template <typename T, typename U>
bool operator==(const SecureAllocator<T>&, const SecureAllocator<U>&) {
  return true;
}

template <typename T, typename U>
bool operator!=(const SecureAllocator<T>&, const SecureAllocator<U>&) {
  return false;
}

/**
 * @brief String type holding its contents in locked, self-wiping memory.
 */
using SecureString =
std::basic_string<char, std::char_traits<char>,
SecureAllocator<char>>;

/**
 * @brief Deleter that zeroes an object before releasing it.
 */
struct SecureDeleter {
  /**
   * @brief Zeroes and deletes the given object.
   * @tparam U Type of the object.
   * @param ptr Pointer to the object to delete; may be @c nullptr.
   */
  template <typename U>
  void operator()(U* ptr) const {
    if (ptr) {
      SecureClear(ptr, sizeof(U));
      delete ptr;
    }
  }
};

/**
 * @brief Unique pointer that wipes its pointee on destruction.
 * @tparam T Type of the managed object.
 */
template <typename T>
using SecureUniquePtr = std::unique_ptr<T, SecureDeleter>;

/**
 * @brief Creates a @c SecureUniquePtr holding a newly constructed object.
 * @tparam T    Type of the object.
 * @tparam Args Constructor argument types.
 * @param args  Constructor arguments.
 * @return The new @c SecureUniquePtr.
 */
template <typename T, typename... Args>
SecureUniquePtr<T> MakeSecureUnique(Args&&... args) {
  return SecureUniquePtr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace util
}  // namespace lock
}  // namespace gxde

#endif  // SRC_UTIL_SECURE_MEMORY_SECURE_ALLOCATOR_H_
