#pragma once

/// @file userver/utils/constexpr_indices.hpp
/// @brief Functions for iterating over a constexpr range of integers

#include <cstddef>
#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <std::size_t... Indices, typename Func>
void DoForEachIndex(std::index_sequence<Indices...>, Func func) {
  static_assert(std::is_trivially_copyable_v<Func>);
  (..., func(std::integral_constant<std::size_t, Indices>{}));
}

template <std::size_t... Indices, typename Func>
void DoWithConstexprIndex(std::index_sequence<Indices...>,
                          std::size_t runtime_index, Func func) {
  static_assert(std::is_trivially_copyable_v<Func>);
  (..., (runtime_index == Indices
             ? func(std::integral_constant<std::size_t, Indices>{})
             : void()));
}

}  // namespace impl

/// Calls `func` with indices from range `0...<Count`, wrapped in
/// `std::integral_constant`.
template <std::size_t Count, typename Func>
void ForEachIndex(Func func) {
  impl::DoForEachIndex(std::make_index_sequence<Count>{}, func);
}

/// Calls `func` with `runtime_index` wrapped in `std::integral_constant`.
template <std::size_t Count, typename Func>
void WithConstexprIndex(std::size_t runtime_index, Func func) {
  impl::DoWithConstexprIndex(std::make_index_sequence<Count>{}, runtime_index,
                             func);
}

}  // namespace utils

USERVER_NAMESPACE_END
