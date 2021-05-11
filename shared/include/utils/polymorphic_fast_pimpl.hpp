#pragma once

/// @file utils/polymorphic_fast_pimpl.hpp
/// @brief @copybrief utils::PolymorphicFastPimpl

#include <new>
#include <type_traits>
#include <utility>

namespace utils {

/**
 * @brief PolymorphicFastPimpl implements pimpl idiom without dynamic
 * allocation for polymorphic types.
 *
 * PolymorphicFastPimpl doesn't require either memory allocation or indirect
 * memory access. But you have to manually set object size when you instantiate
 * PolymorphicFastPimpl.
 *
 * This is an extension of FastPimpl allowing storage of derived types.
 * It is intended to use with interfaces having multiple implementations.
 */
template <class Base, size_t Size, size_t Alignment, bool Strict = false>
class PolymorphicFastPimpl final {
 public:
  PolymorphicFastPimpl(const PolymorphicFastPimpl&) = delete;
  PolymorphicFastPimpl(PolymorphicFastPimpl&&) = delete;
  PolymorphicFastPimpl& operator=(const PolymorphicFastPimpl&) = delete;
  PolymorphicFastPimpl& operator=(PolymorphicFastPimpl&&) = delete;

  template <class Derived, class... Args>
  explicit PolymorphicFastPimpl(
      std::in_place_type_t<Derived>,
      Args&&... args) noexcept(noexcept(Derived(std::declval<Args>()...))) {
    Validator<Derived, sizeof(Derived), alignof(Derived)>::Validate();
    new (As<Derived>()) Derived(std::forward<Args>(args)...);
  }

  ~PolymorphicFastPimpl() noexcept {
    static_assert(std::has_virtual_destructor_v<Base>,
                  "Base does not have a virtual destructor");
    As<Base>()->~Base();
  }

  Base* operator->() noexcept { return As<Base>(); }

  const Base* operator->() const noexcept { return As<Base>(); }

  Base& operator*() noexcept { return *As<Base>(); }

  const Base& operator*() const noexcept { return *As<Base>(); }

  Base* Get() noexcept { return As<Base>(); }

  const Base* Get() const noexcept { return As<Base>(); }

 private:
  // Separate class for better diagnostics: with it actual sizes are visible in
  // compiler error message.
  template <typename Derived, std::size_t ActualSize,
            std::size_t ActualAlignment>
  struct Validator {
    static void Validate() noexcept {
      static_assert(std::is_base_of_v<Base, Derived>,
                    "incorrect Derived: is not derived from Base");
      static_assert(Size >= ActualSize,
                    "incorrect Size: less than sizeof(Derived)");
      static_assert(Size == ActualSize || !Strict,
                    "incorrect Size: sizeof(Derived) mismatch in strict mode");
      static_assert(Alignment % ActualAlignment == 0,
                    "incorrect Alignment: alignof(Derived) mismatch");
    }
  };

  template <class T>
  T* As() noexcept {
    return reinterpret_cast<T*>(&storage_);
  }

  template <class T>
  const T* As() const noexcept {
    return reinterpret_cast<const T*>(&storage_);
  }

  std::aligned_storage_t<Size, Alignment> storage_;
};

}  // namespace utils
