#pragma once

#include <cstddef>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

#if defined(__cpp_lib_hardware_interference_size)
inline constexpr std::size_t kDestructiveInterferenceSize =
    std::hardware_destructive_interference_size;
#elif defined(__s390__) || defined(__s390x__)
inline constexpr std::size_t kDestructiveInterferenceSize = 256;
#elif defined(powerpc) || defined(__powerpc__) || defined(__ppc__)
inline constexpr std::size_t kDestructiveInterferenceSize = 128;
#else
inline constexpr std::size_t kDestructiveInterferenceSize = 64;
#endif

/// Protects the surrounding data from the destructive interference of atomics
/// inside, and vice-versa.
template <typename T>
class alignas(kDestructiveInterferenceSize) InterferenceShield final {
 public:
  template <typename... Args>
  constexpr InterferenceShield(Args&&... args)
      : value_(std::forward<Args>(args)...) {}

  constexpr T& operator*() { return value_; }
  constexpr const T& operator*() const { return value_; }
  constexpr T* operator->() { return &value_; }
  constexpr const T* operator->() const { return &value_; }

 private:
  T value_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
