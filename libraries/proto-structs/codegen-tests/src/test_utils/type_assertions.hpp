#pragma once

#include <cstddef>
#include <type_traits>

#include <boost/pfr/core.hpp>

USERVER_NAMESPACE_BEGIN

namespace impl {

template <typename T>
constexpr std::size_t GetFieldCount() {
    return boost::pfr::tuple_size<T>::value;
}

}  // namespace impl

template <typename Field, typename Expected>
constexpr void AssertFieldType() {
    static_assert(std::is_same_v<Field, Expected>, "Must be equal");
}

template <typename T, std::size_t Count>
constexpr void AssertFieldCount() {
    static_assert(impl::GetFieldCount<T>() == Count);
}

USERVER_NAMESPACE_END
