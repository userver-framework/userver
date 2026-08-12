#pragma once

/// @file userver/storages/postgres/io/span.hpp
/// @brief std::span I/O support
/// @ingroup userver_postgres_parse_and_format

#include <span>

#include <userver/storages/postgres/io/array_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io::traits {

template <typename T, std::size_t Extent>
struct IsCompatibleContainer<std::span<T, Extent>> : std::true_type {};

namespace detail {

struct SpanHasNoParser;

}  // namespace detail

/// Override the array Input<T> = ArrayBinaryParser<T> that would otherwise be
/// auto-selected via IsCompatibleContainer.
template <typename T, std::size_t Extent>
struct Input<std::span<T, Extent>> {
    using type = detail::SpanHasNoParser;
};

}  // namespace storages::postgres::io::traits

USERVER_NAMESPACE_END
