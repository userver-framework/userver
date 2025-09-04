//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// This header is unguarded on purpose. This header is an implementation detail of move_only_function.h
// and generates multiple versions of std::move_only_function

#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

#include "move_only_function_common.h"
#include "pointer_int_pair.h"
#include "small_buffer.h"

#pragma GCC system_header

#ifndef USERVER_IN_MOVE_ONLY_FUNCTION_H
#  error This header should only be included from move_only_function.h
#endif

#ifndef USERVER_MOVE_ONLY_FUNCTION_CV
#  define USERVER_MOVE_ONLY_FUNCTION_CV
#endif

#ifndef USERVER_MOVE_ONLY_FUNCTION_REF
#  define USERVER_MOVE_ONLY_FUNCTION_REF
#  define USERVER_MOVE_ONLY_FUNCTION_INVOKE_QUALS USERVER_MOVE_ONLY_FUNCTION_CV&
#else
#  define USERVER_MOVE_ONLY_FUNCTION_INVOKE_QUALS USERVER_MOVE_ONLY_FUNCTION_CV USERVER_MOVE_ONLY_FUNCTION_REF
#endif

#ifndef USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT
#  define USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT false
#endif

#define USERVER_MOVE_ONLY_FUNCTION_CVREF USERVER_MOVE_ONLY_FUNCTION_CV USERVER_MOVE_ONLY_FUNCTION_REF

namespace function_backports {

template <class...>
class move_only_function;

template <class _ReturnT, class... _ArgTypes>
class move_only_function<_ReturnT(_ArgTypes...)
    USERVER_MOVE_ONLY_FUNCTION_CVREF noexcept(USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT)> {
private:
  static constexpr size_t __buffer_size_      = 3 * sizeof(void*);
  static constexpr size_t __buffer_alignment_ = alignof(void*);
  using _BufferT                              = __small_buffer<__buffer_size_, __buffer_alignment_>;

  using _TrivialVTable    = _MoveOnlyFunctionTrivialVTable<_BufferT, _ReturnT, _ArgTypes...>;
  using _NonTrivialVTable = _MoveOnlyFunctionNonTrivialVTable<_BufferT, _ReturnT, _ArgTypes...>;

  template <class _Functor>
  static constexpr _TrivialVTable __trivial_vtable_ = {
      .__call_ = [](_BufferT& __buffer, _ArgTypes... __args) noexcept(USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT) -> _ReturnT {
        return std::invoke(
            static_cast<_Functor USERVER_MOVE_ONLY_FUNCTION_INVOKE_QUALS>(*__buffer.template __get<_Functor>()),
            std::forward<_ArgTypes>(__args)...);
      }};

  template <class _Functor>
  static constexpr _NonTrivialVTable __non_trivial_vtable_{
      __trivial_vtable_<_Functor>,
      [](_BufferT& __buffer) noexcept -> void {
        std::destroy_at(__buffer.template __get<_Functor>());
        __buffer.template __dealloc<_Functor>();
      },
  };

  template <class _Functor>
  __pointer_bool_pair<const _TrivialVTable*> __get_vptr() {
    if constexpr (_BufferT::template __fits_in_buffer<_Functor> && std::is_trivially_destructible_v<_Functor>) {
      return {&__trivial_vtable_<_Functor>, false};
    } else {
      return {&__non_trivial_vtable_<_Functor>, true};
    }
  }

  template <class _VT>
  static constexpr bool __is_callable_from =
#if USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT
      std::is_nothrow_invocable_r_v<_ReturnT, std::decay_t<_VT> USERVER_MOVE_ONLY_FUNCTION_CVREF, _ArgTypes...> &&
      std::is_nothrow_invocable_r_v<_ReturnT, std::decay_t<_VT> USERVER_MOVE_ONLY_FUNCTION_INVOKE_QUALS, _ArgTypes...>;
#else
      std::is_invocable_r_v<_ReturnT, std::decay_t<_VT> USERVER_MOVE_ONLY_FUNCTION_CVREF, _ArgTypes...> &&
      std::is_invocable_r_v<_ReturnT, std::decay_t<_VT> USERVER_MOVE_ONLY_FUNCTION_INVOKE_QUALS, _ArgTypes...>;
#endif

  template <class _Func, class... _Args>
  void __construct(_Args&&... __args) {
    static_assert(std::is_constructible_v<std::decay_t<_Func>, _Func>);

    using _StoredFunc = std::decay_t<_Func>;
    __vtable_         = __get_vptr<_StoredFunc>();
    __buffer_.template __construct<_StoredFunc>(std::forward<_Args>(__args)...);
  }

  void __reset() {
    if (__vtable_.__get_value())
      static_cast<const _NonTrivialVTable*>(__vtable_.__get_ptr())->__destroy_(__buffer_);
    __vtable_ = {};
  }

public:
  using result_type = _ReturnT;

  // [func.wrap.move.ctor]
  move_only_function() noexcept = default;
  move_only_function(std::nullptr_t) noexcept {}
  move_only_function(move_only_function&& __other) noexcept
      : __vtable_(__other.__vtable_), __buffer_(std::move(__other.__buffer_)) {
    __other.__vtable_ = {};
  }

  template <class _Func,
            std::enable_if_t<
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<_Func>>, move_only_function> &&
                !__is_inplace_type<_Func>::value &&
                __is_callable_from<_Func>, int> = 0>
  move_only_function(_Func&& __func) noexcept {
    using _StoredFunc = std::decay_t<_Func>;

    if constexpr ((std::is_pointer_v<_StoredFunc> && std::is_function_v<std::remove_pointer_t<_StoredFunc>>) ||
                  std::is_member_function_pointer_v<_StoredFunc>) {
      if (__func != nullptr) {
        __vtable_ = __get_vptr<_StoredFunc>();
        static_assert(_BufferT::template __fits_in_buffer<_StoredFunc>);
        __buffer_.template __construct<_StoredFunc>(std::forward<_Func>(__func));
      }
    } else if constexpr (__is_move_only_function<_StoredFunc>::value) {
      if (__func) {
        __vtable_ = std::exchange(__func.__vtable_, {});
        __buffer_ = std::move(__func.__buffer_);
      }
    } else {
      __construct<_Func>(std::forward<_Func>(__func));
    }
  }

  template <class _Func, class... _Args,
            std::enable_if_t<
                std::is_constructible_v<std::decay_t<_Func>, _Args...> &&
                __is_callable_from<_Func>, int> = 0>
  explicit move_only_function(std::in_place_type_t<_Func>, _Args&&... __args) {
    static_assert(std::is_same_v<std::decay_t<_Func>, _Func>);
    __construct<_Func>(std::forward<_Args>(__args)...);
  }

  template <class _Func, class _InitListType, class... _Args,
            std::enable_if_t<
                std::is_constructible_v<std::decay_t<_Func>, std::initializer_list<_InitListType>&, _Args...> &&
                __is_callable_from<_Func>, int> = 0>
  explicit move_only_function(
      std::in_place_type_t<_Func>, std::initializer_list<_InitListType> __il, _Args&&... __args) {
    static_assert(std::is_same_v<std::decay_t<_Func>, _Func>);
    __construct<_Func>(__il, std::forward<_Args>(__args)...);
  }

  move_only_function& operator=(move_only_function&& __other) noexcept {
    move_only_function(std::move(__other)).swap(*this);
    return *this;
  }

  move_only_function& operator=(std::nullptr_t) noexcept {
    __reset();
    return *this;
  }

  template <class _Func,
            std::enable_if_t<
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<_Func>>, move_only_function> &&
                !__is_inplace_type<_Func>::value &&
                __is_callable_from<_Func>, int> = 0>
  move_only_function& operator=(_Func&& __func) {
    move_only_function(std::forward<_Func>(__func)).swap(*this);
    return *this;
  }

  ~move_only_function() { __reset(); }

  // [func.wrap.move.inv]
  explicit operator bool() const noexcept { return __vtable_.__get_ptr() != nullptr; }

  _ReturnT operator()(_ArgTypes... __args) USERVER_MOVE_ONLY_FUNCTION_CVREF
      noexcept(USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT) {
    assert(static_cast<bool>(*this) && "Tried to call a disengaged move_only_function");
    const auto __call = static_cast<_ReturnT (*)(_BufferT&, _ArgTypes...)>(__vtable_.__get_ptr()->__call_);
    return __call(__buffer_, std::forward<_ArgTypes>(__args)...);
  }

  // [func.wrap.move.util]
  void swap(move_only_function& __other) noexcept {
    std::swap(__vtable_, __other.__vtable_);
    std::swap(__buffer_, __other.__buffer_);
  }

  friend void swap(move_only_function& __lhs, move_only_function& __rhs) noexcept {
    __lhs.swap(__rhs);
  }

  friend bool operator==(const move_only_function& __func, std::nullptr_t) noexcept { return !__func; }

private:
  __pointer_bool_pair<const _TrivialVTable*> __vtable_ = {};
  mutable _BufferT __buffer_;

  template <class...>
  friend class move_only_function;
};

#undef USERVER_MOVE_ONLY_FUNCTION_CV
#undef USERVER_MOVE_ONLY_FUNCTION_REF
#undef USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT
#undef USERVER_MOVE_ONLY_FUNCTION_INVOKE_QUALS
#undef USERVER_MOVE_ONLY_FUNCTION_CVREF

}  // namespace function_backports
