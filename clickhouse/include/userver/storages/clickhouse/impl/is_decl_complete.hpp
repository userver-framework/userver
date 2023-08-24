#pragma once

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::utils {

namespace impl {

template <typename T, std::size_t = sizeof(T)>
std::true_type IsCompleteImpl(T*);

std::false_type IsCompleteImpl(...);

}  // namespace impl

template <typename T>
using IsDeclComplete = decltype(impl::IsCompleteImpl(std::declval<T*>()));

}  // namespace storages::clickhouse::utils

USERVER_NAMESPACE_END
