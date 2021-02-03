#pragma once

#include <type_traits>

#include <boost/pfr/core.hpp>
#include <boost/pfr/tuple_size.hpp>

#include <cache/dump/operations.hpp>

namespace cache::dump {

namespace impl {

template <typename T, std::size_t... Indices>
constexpr bool AreAllDumpable(std::index_sequence<Indices...>) {
  return (kIsDumpable<boost::pfr::tuple_element_t<Indices, T>> && ...);
}

template <typename T>
constexpr bool IsDumpableAggregate() {
  if constexpr (std::is_aggregate_v<T> &&
                std::is_move_constructible_v<std::remove_all_extents_t<T>> &&
                std::is_move_assignable_v<std::remove_all_extents_t<T>>) {
    constexpr auto kSize = boost::pfr::tuple_size_v<T>;
    return AreAllDumpable<T>(std::make_index_sequence<kSize>{});
  } else {
    return false;
  }
}

template <typename T, std::size_t... Indices>
T ReadAggregate(Reader& reader, std::index_sequence<Indices...>) {
  // `Read`s are guaranteed to occur left-to-right in brace-init
  return T{reader.Read<boost::pfr::tuple_element_t<Indices, T>>()...};
}

}  // namespace impl

/// @{
/// Aggregates support
template <typename T>
std::enable_if_t<impl::IsDumpableAggregate<T>()> Write(Writer& writer,
                                                       const T& value) {
  boost::pfr::for_each_field(
      value, [&writer](const auto& field) { writer.Write(field); });
}

template <typename T>
std::enable_if_t<impl::IsDumpableAggregate<T>(), T> Read(Reader& reader,
                                                         To<T>) {
  constexpr auto kSize = boost::pfr::tuple_size_v<T>;
  return impl::ReadAggregate<T>(reader, std::make_index_sequence<kSize>{});
}
/// @}

}  // namespace cache::dump
