#pragma once

#include <userver/utils/meta.hpp>

#include <userver/storages/clickhouse/impl/is_decl_complete.hpp>
#include <userver/storages/clickhouse/io/io_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::traits {

template <typename T>
inline constexpr bool kIsMappedToClickhouse =
    utils::IsDeclComplete<CppToClickhouse<T>>::value;

template <typename T>
auto Inserter(T& container) {
  return meta::Inserter(container);
}

template <typename T>
inline constexpr bool kIsReservable = meta::kIsReservable<T>;

template <typename T>
inline constexpr bool kIsSizeable = meta::kIsSizable<T>;

template <typename T>
inline constexpr bool kIsRange = meta::kIsRange<T>;

}  // namespace storages::clickhouse::io::traits

USERVER_NAMESPACE_END
