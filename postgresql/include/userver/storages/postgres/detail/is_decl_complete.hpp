#pragma once

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::utils {

namespace detail {
template <typename T, std::size_t = sizeof(T)>
std::true_type IsCompleteImpl(T*);

std::false_type IsCompleteImpl(...);
}  // namespace detail

template <typename T>
using IsDeclComplete = decltype(detail::IsCompleteImpl(std::declval<T*>()));

}  // namespace storages::postgres::utils

USERVER_NAMESPACE_END
