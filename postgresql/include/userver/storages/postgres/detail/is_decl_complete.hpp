#pragma once

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace storages {
namespace postgres {
namespace utils {

namespace detail {
template <typename T, std::size_t = sizeof(T)>
std::true_type IsCompleteImpl(T*);

std::false_type IsCompleteImpl(...);
}  // namespace detail

template <typename T>
using IsDeclComplete = decltype(detail::IsCompleteImpl(std::declval<T*>()));

}  // namespace utils
}  // namespace postgres
}  // namespace storages

USERVER_NAMESPACE_END
