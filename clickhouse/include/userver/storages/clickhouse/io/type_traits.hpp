#pragma once

#include <string>
#include <type_traits>

#include <userver/storages/clickhouse/impl/is_decl_complete.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::traits {

template <typename T>
inline constexpr bool kIsMappedToClickhouse = utils::IsDeclComplete<T>::value;

}  // namespace storages::clickhouse::io::traits

USERVER_NAMESPACE_END
