//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#pragma GCC system_header

// A __pointer_int_pair is a pair of a pointer and an integral type. The lower bits of the pointer that are free
// due to the alignment requirement of the pointee are used to store the integral type.
//
// This imposes a constraint on the number of bits available for the integral type -- the integral type can use
// at most log2(alignof(T)) bits. This technique allows storing the integral type without additional storage
// beyond that of the pointer itself, at the cost of some bit twiddling.

namespace function_backports {

template <class _Tp>
struct _PointerLikeTraits;

template <class _Tp>
struct _PointerLikeTraits<_Tp*> {
  static constexpr std::size_t __low_bits_available = []() constexpr {
    if constexpr (std::is_void_v<_Tp>) {
      return 0;
    } else {
      return __builtin_ctzll(alignof(_Tp));
    }
  }();

  static std::uintptr_t __to_uintptr(_Tp* __ptr) { return reinterpret_cast<std::uintptr_t>(__ptr); }
  static _Tp* __to_pointer(std::uintptr_t __ptr) { return reinterpret_cast<_Tp*>(__ptr); }
};

enum class __integer_width : std::size_t {};

template <class _Pointer, class _IntType, __integer_width __int_bit_count>
class __pointer_int_pair {
  using _PointerTraits = _PointerLikeTraits<_Pointer>;

  static constexpr auto __int_width = static_cast<std::size_t>(__int_bit_count);

  static_assert(__int_width <= _PointerTraits::__low_bits_available,
                "Not enough bits available for requested bit count");
  static_assert(std::is_integral_v<_IntType>, "_IntType has to be an integral type");
  static_assert(std::is_unsigned_v<_IntType>, "__pointer_int_pair doesn't work for signed types");

  static constexpr std::size_t __extra_bits  = _PointerTraits::__low_bits_available - __int_width;
  static constexpr std::uintptr_t __int_mask = static_cast<std::uintptr_t>(1 << _PointerTraits::__low_bits_available) - 1;
  static constexpr std::uintptr_t __ptr_mask = ~__int_mask;

  std::uintptr_t __value_ = 0;

public:
  __pointer_int_pair()                                     = default;
  __pointer_int_pair(const __pointer_int_pair&)            = default;
  __pointer_int_pair(__pointer_int_pair&&)                 = default;
  __pointer_int_pair& operator=(const __pointer_int_pair&) = default;
  __pointer_int_pair& operator=(__pointer_int_pair&&)      = default;
  ~__pointer_int_pair()                                    = default;

  __pointer_int_pair(_Pointer __ptr_value, _IntType __int_value)
      : __value_(_PointerTraits::__to_uintptr(__ptr_value) | (__int_value << __extra_bits)) {}

  _IntType __get_value() const { return (__value_ & __int_mask) >> __extra_bits; }
  _Pointer __get_ptr() const { return _PointerTraits::__to_pointer(__value_ & __ptr_mask); }

  template <class>
  friend struct _PointerLikeTraits;
};

template <class _Pointer, __integer_width __int_bit_count, class _IntType>
struct _PointerLikeTraits<__pointer_int_pair<_Pointer, _IntType, __int_bit_count>> {
private:
  using _PointerIntPair = __pointer_int_pair<_Pointer, _IntType, __int_bit_count>;

  static_assert(_PointerLikeTraits<_Pointer>::__low_bits_available >= static_cast<std::size_t>(__int_bit_count));

public:
  static constexpr size_t __low_bits_available =
      _PointerLikeTraits<_Pointer>::__low_bits_available - static_cast<std::size_t>(__int_bit_count);

  static std::uintptr_t __to_uintptr(_PointerIntPair __ptr) { return __ptr.__value_; }

  static _PointerIntPair __to_pointer(std::uintptr_t __ptr) {
    _PointerIntPair __tmp{};
    __tmp.__value_ = __ptr;
    return __tmp;
  }
};

template <class _Pointer>
using __pointer_bool_pair = __pointer_int_pair<_Pointer, bool, __integer_width{1}>;

// Make __pointer_int_pair tuple-like

template <size_t __i, class _Pointer, class _IntType, __integer_width __int_bit_count>
auto get(__pointer_int_pair<_Pointer, _IntType, __int_bit_count> __pair) {
  if constexpr (__i == 0) {
    return __pair.__get_ptr();
  } else if constexpr (__i == 1) {
    return __pair.__get_value();
  } else {
    static_assert(__i == 0, "Index out of bounds");
  }
}

}  // namespace function_backports

template <class _Pointer, class _IntType, function_backports::__integer_width __int_bit_count>
struct std::tuple_size<function_backports::__pointer_int_pair<_Pointer, _IntType, __int_bit_count>> : std::integral_constant<std::size_t, 2> {};

template <class _Pointer, class _IntType, function_backports::__integer_width __int_bit_count>
struct std::tuple_element<0, function_backports::__pointer_int_pair<_Pointer, _IntType, __int_bit_count>> {
  using type = _Pointer;
};

template <class _Pointer, class _IntType, function_backports::__integer_width __int_bit_count>
struct std::tuple_element<1, function_backports::__pointer_int_pair<_Pointer, _IntType, __int_bit_count>> {
  using type = _IntType;
};
