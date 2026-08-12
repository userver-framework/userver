//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include <type_traits>
#include <utility>

#pragma GCC system_header

namespace function_backports {

template <class...>
class move_only_function;

template <class>
struct __is_move_only_function : std::false_type {};

template <class... _Args>
struct __is_move_only_function<move_only_function<_Args...>> : std::true_type {};

template <class _BufferT, class _ReturnT, class... _ArgTypes>
struct _MoveOnlyFunctionTrivialVTable {
  using _CallFunc = _ReturnT(_BufferT&, _ArgTypes...);

  _CallFunc* __call_;
};

template <class _BufferT, class _ReturnT, class... _ArgTypes>
struct _MoveOnlyFunctionNonTrivialVTable : _MoveOnlyFunctionTrivialVTable<_BufferT, _ReturnT, _ArgTypes...> {
  using _DestroyFunc = void(_BufferT&) noexcept;

  _DestroyFunc* __destroy_;
};

template <class _Tp> struct __is_inplace_type_imp : std::false_type {};
template <class _Tp> struct __is_inplace_type_imp<std::in_place_type_t<_Tp>> : std::true_type {};

template <class _Tp>
using __is_inplace_type = __is_inplace_type_imp<std::remove_cv_t<std::remove_reference_t<_Tp>>>;

}  // namespace function_backports
