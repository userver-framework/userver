//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

#pragma GCC system_header

// __small_buffer is a helper class to perform the well known SBO (small buffer optimization). It is mainly useful to
// allow type-erasing classes like move_only_function to store small objects in a local buffer without requiring an
// allocation.
//
// This small buffer class only allows storing trivially relocatable objects inside the local storage to allow
// __small_buffer to be trivially relocatable itself. Since the buffer doesn't know what's stored inside it, the user
// has to manage the object's lifetime, in particular the destruction of the object.

namespace function_backports {

template <std::size_t _BufferSize, std::size_t _BufferAlignment>
class __small_buffer {
  static_assert(_BufferSize > 0 && _BufferAlignment > 0);
public:
  template <class _Tp, class _Decayed = std::decay_t<_Tp>>
  static constexpr bool __fits_in_buffer =
      std::is_trivially_move_constructible_v<_Decayed> && std::is_trivially_destructible_v<_Decayed> &&
      sizeof(_Decayed) <= _BufferSize && alignof(_Decayed) <= _BufferAlignment;

  __small_buffer()           = default;
  __small_buffer(const __small_buffer&)            = delete;
  __small_buffer& operator=(const __small_buffer&) = delete;
  ~__small_buffer()          = default;

  // Relocates the buffer - __delete() should never be called on a moved-from __small_buffer
  __small_buffer(__small_buffer&&)            = default;
  __small_buffer& operator=(__small_buffer&&) = default;

  template <class _Stored>
  _Stored* __get() {
    if constexpr (__fits_in_buffer<_Stored>)
      return std::launder(reinterpret_cast<_Stored*>(__buffer_));
    else
      return *std::launder(reinterpret_cast<_Stored**>(__buffer_));
  }

  template <class _Stored>
  _Stored* __alloc() {
    if constexpr (__fits_in_buffer<_Stored>) {
      return std::launder(reinterpret_cast<_Stored*>(__buffer_));
    } else {
      std::byte* __allocation = static_cast<std::byte*>(::operator new[](sizeof(_Stored), std::align_val_t{alignof(_Stored)}));
      ::new (__buffer_) auto(__allocation);
      return std::launder(reinterpret_cast<_Stored*>(__allocation));
    }
  }

  template <class _Stored>
  void __dealloc() noexcept {
    if constexpr (!__fits_in_buffer<_Stored>)
      ::operator delete[](*reinterpret_cast<void**>(__buffer_), std::align_val_t{alignof(_Stored)});
  }

  template <class _Stored, class... _Args>
  void __construct(_Args&&... __args) {
    _Stored* __buffer = __alloc<_Stored>();
    try {
      ::new (__buffer) _Stored(std::forward<_Args>(__args)...);
    } catch (...) {
      __dealloc<_Stored>();
    }
  }

private:
  alignas(_BufferAlignment) std::byte __buffer_[_BufferSize];
};

}  // namespace function_backports
