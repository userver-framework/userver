#pragma once

#include <string>
#include <type_traits>

#include <userver/storages/clickhouse/impl/is_decl_complete.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::traits {

template <typename T>
inline constexpr bool kIsMappedToClickhouse = utils::IsDeclComplete<T>::value;

template <typename T>
auto Inserter(T& container) {
  return meta::Inserter(container);
}

template <typename T>
inline constexpr bool kIsReservable = meta::kIsReservable<T>;

}  // namespace storages::clickhouse::io::traits

USERVER_NAMESPACE_END
