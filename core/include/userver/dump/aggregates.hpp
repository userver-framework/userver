#pragma once

/// @file userver/dump/aggregates.hpp
/// @brief Dumping support for aggregates
///
/// @ingroup userver_dump_read_write

#include <type_traits>

#include <boost/pfr/core.hpp>
#include <boost/pfr/tuple_size.hpp>

#include <userver/dump/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

template <typename T>
struct IsDumpedAggregate {};

namespace impl {

// Only the non-specialized IsDumpedAggregate struct is defined,
// the specializations are declared without a definition
template <typename T>
using IsNotDumpedAggregate = decltype(sizeof(IsDumpedAggregate<T>));

template <typename T, std::size_t... Indices>
constexpr bool AreAllDumpable(std::index_sequence<Indices...>) {
  return (kIsDumpable<boost::pfr::tuple_element_t<Indices, T>> && ...);
}

template <typename T>
constexpr bool IsDumpableAggregate() {
  if constexpr (std::is_aggregate_v<T> &&
                !meta::kIsDetected<IsNotDumpedAggregate, T>) {
    constexpr auto kSize = boost::pfr::tuple_size_v<T>;
    static_assert(
        AreAllDumpable<T>(std::make_index_sequence<kSize>{}),
        "One of the member of an aggregate that was marked as dumpable "
        "with dump::IsDumpedAggregate is not dumpable. Did you forget "
        "to include <userver/dump/common.hpp>, "
        "<userver/dump/common_containers.hpp> or some other header with "
        "Read and Write functions declarations?");
    return true;
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

/// @brief Aggregates dumping support
///
/// To enable dumps and loads for an aggregate, add in the global namespace:
///
/// @code
/// template <>
/// struct dump::IsDumpedAggregate<MyStruct>;
/// @endcode
///
/// @warning Don't forget to increment format-version if data layout changes
template <typename T>
std::enable_if_t<impl::IsDumpableAggregate<T>()> Write(Writer& writer,
                                                       const T& value) {
  boost::pfr::for_each_field(
      value, [&writer](const auto& field) { writer.Write(field); });
}

/// @brief Aggregates deserialization from dump support
///
/// To enable dumps and loads for an aggregate, add in the global namespace:
///
/// @code
/// template <>
/// struct dump::IsDumpedAggregate<MyStruct>;
/// @endcode
///
/// @warning Don't forget to increment format-version if data layout changes
template <typename T>
std::enable_if_t<impl::IsDumpableAggregate<T>(), T> Read(Reader& reader,
                                                         To<T>) {
  constexpr auto kSize = boost::pfr::tuple_size_v<T>;
  return impl::ReadAggregate<T>(reader, std::make_index_sequence<kSize>{});
}

}  // namespace dump

USERVER_NAMESPACE_END
