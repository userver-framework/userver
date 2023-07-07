#pragma once

/// @file userver/compiler/select.hpp
/// @brief Utilities for selection of platform specific values

#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace compiler {

namespace impl {
[[noreturn]] void Abort() noexcept;
}

/// @brief Selects the proper value for the current compiler and standard
/// library.
///
/// If proper value for the standard library was set, prefers it over the
/// generic x32/x64 values.
///
/// ## Example usage:
/// @snippet utils/widget_fast_pimpl_test.hpp  FastPimpl - header
template <typename T>
class SelectValue final {
 public:
  constexpr SelectValue() = default;

  constexpr SelectValue& ForLibCpp64(T value) noexcept {
    return Set(Bits::k64, StdLibs::kCpp, value);
  }

  constexpr SelectValue& ForLibStdCpp64(T value) noexcept {
    return Set(Bits::k64, StdLibs::kStdCpp, value);
  }

  constexpr SelectValue& ForLibCpp32(T value) noexcept {
    return Set(Bits::k32, StdLibs::kCpp, value);
  }

  constexpr SelectValue& ForLibStdCpp32(T value) noexcept {
    return Set(Bits::k32, StdLibs::kStdCpp, value);
  }

  constexpr SelectValue& For64Bit(T value) noexcept {
    return Set(Bits::k64, StdLibs::kAny, value);
  }

  constexpr SelectValue& For32Bit(T value) noexcept {
    return Set(Bits::k32, StdLibs::kAny, value);
  }

  constexpr operator T() const noexcept {
    if (has_stdlib_value_) {
      return stdlib_value_;
    } else if (has_bits_value_) {
      return bits_value_;
    } else {
      compiler::impl::Abort();
    }
  }

 private:
  enum class Bits {
    k32,
    k64,
  };

  enum class StdLibs {
    kAny,
    kStdCpp,
    kCpp,
  };

  constexpr SelectValue& Set(Bits bits, StdLibs lib, T value) noexcept {
    constexpr auto kBits = (sizeof(void*) == 8 ? Bits::k64 : Bits::k32);
    constexpr auto kLib =
#if defined(_LIBCPP_VERSION)
        StdLibs::kCpp
#else
        StdLibs::kStdCpp
#endif
        ;

    if (bits == kBits) {
      if (lib == kLib) {
        has_stdlib_value_ = true;
        stdlib_value_ = value;
      } else if (lib == StdLibs::kAny) {
        has_bits_value_ = true;
        bits_value_ = value;
      }
    }

    return *this;
  }

  T stdlib_value_ = {};
  T bits_value_ = {};
  bool has_stdlib_value_ = false;
  bool has_bits_value_ = false;
};

/// @brief Alias for std::size_t values selection for the current compiler and
/// standard library.
using SelectSize = SelectValue<std::size_t>;

}  // namespace compiler

USERVER_NAMESPACE_END
