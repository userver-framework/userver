#pragma once

/// @file userver/utils/fast_pimpl.hpp
/// @brief @copybrief utils::FastPimpl

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Helper constant to use with FastPimpl
inline constexpr bool kStrictMatch = true;

/// @ingroup userver_containers
///
/// @brief Implements pimpl idiom without dynamic memory allocation.
///
/// FastPimpl doesn't require either memory allocation or indirect memory
/// access. But you have to manually set object size when you instantiate
/// FastPimpl.
///
/// ## Example usage:
/// Take your class with pimpl via smart pointer and
/// replace the smart pointer with utils::FastPimpl<Impl, Size, Alignment>
/// @snippet utils/widget_fast_pimpl_test.hpp  FastPimpl - header
///
/// If the Size and Alignment are unknown - just put a random ones and
/// the compiler would show the right ones in the error message:
/// @code
/// In instantiation of 'void FastPimpl<T, Size, Alignment>::Validate()
/// [with int ActualSize = 1; int ActualAlignment = 8; T = sample::Widget;
/// int Size = 8; int Alignment = 8]'
/// @endcode
///
/// Change the initialization in source file to not allocate for pimpl
/// @snippet utils/widget_fast_pimpl_test.cpp  FastPimpl - source
///
/// Done! Now you can use the header without exposing the implementation
/// details:
/// @snippet utils/fast_pimpl_test.cpp  FastPimpl - usage
template <class T, std::size_t Size, std::size_t Alignment, bool Strict = false>
class FastPimpl final {
 public:
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,performance-noexcept-move-constructor)
  FastPimpl(FastPimpl&& v) noexcept(noexcept(T(std::declval<T>())))
      : FastPimpl(std::move(*v)) {}

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  FastPimpl(const FastPimpl& v) noexcept(noexcept(T(std::declval<const T&>())))
      : FastPimpl(*v) {}

  // NOLINTNEXTLINE(bugprone-unhandled-self-assignment,cert-oop54-cpp)
  FastPimpl& operator=(const FastPimpl& rhs) noexcept(
      noexcept(std::declval<T&>() = std::declval<const T&>())) {
    *AsHeld() = *rhs;
    return *this;
  }

  FastPimpl& operator=(FastPimpl&& rhs) noexcept(
      // NOLINTNEXTLINE(performance-noexcept-move-constructor)
      noexcept(std::declval<T&>() = std::declval<T>())) {
    *AsHeld() = std::move(*rhs);
    return *this;
  }

  template <typename... Args>
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  explicit FastPimpl(Args&&... args) noexcept(
      noexcept(T(std::declval<Args>()...))) {
    ::new (AsHeld()) T(std::forward<Args>(args)...);
  }

  T* operator->() noexcept { return AsHeld(); }

  const T* operator->() const noexcept { return AsHeld(); }

  T& operator*() noexcept { return *AsHeld(); }

  const T& operator*() const noexcept { return *AsHeld(); }

  ~FastPimpl() noexcept {
    Validate<sizeof(T), alignof(T)>();
    AsHeld()->~T();
  }

 private:
  // Use a template to make actual sizes visible in the compiler error message.
  template <std::size_t ActualSize, std::size_t ActualAlignment>
  static void Validate() noexcept {
    static_assert(Size >= ActualSize, "invalid Size: Size >= sizeof(T) failed");
    static_assert(!Strict || Size == ActualSize,
                  "invalid Size: Size == sizeof(T) failed");

    static_assert(Alignment % ActualAlignment == 0,
                  "invalid Alignment: Alignment % alignof(T) == 0 failed");
    static_assert(!Strict || Alignment == ActualAlignment,
                  "invalid Alignment: Alignment == alignof(T) failed");
  }

  alignas(Alignment) std::byte storage_[Size];

  T* AsHeld() noexcept { return reinterpret_cast<T*>(&storage_); }

  const T* AsHeld() const noexcept {
    return reinterpret_cast<const T*>(&storage_);
  }
};

}  // namespace utils

USERVER_NAMESPACE_END
