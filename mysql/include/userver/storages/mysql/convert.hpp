#pragma once

/// @file userver/storages/mysql/convert.hpp
/// @brief MySQL value conversion tag `To` and helper `DoConvert`

#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::convert {

template <typename T>
struct To final {};

namespace impl {

template <typename T, typename DbType>
concept HasConvert = requires(DbType&& from) { Convert(std::forward<DbType>(from), convert::To<T>{}); };

}  // namespace impl

template <typename T, typename DbType>
T DoConvert(DbType&& from) {
    static_assert(
        // TODO : better wording
        impl::HasConvert<T, DbType>,
        "There is no 'T Convert(From&&, storages::mysql::convert::To<T>)' in "
        "neither namespace of 'T' or `storages::mysql::convert`"
    );

    return Convert(std::forward<DbType>(from), To<T>{});
}

}  // namespace storages::mysql::convert

USERVER_NAMESPACE_END
