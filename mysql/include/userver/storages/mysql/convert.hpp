#pragma once

#include <type_traits>
#include <utility>

#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::convert {

template <typename T>
struct To final {};

namespace impl {

template <typename T, typename DbType>
using HasConvert =
    decltype(Convert(std::declval<DbType&&>(), convert::To<T>{}));

template <typename T, typename DbType>
inline constexpr bool kHasConvert = meta::kIsDetected<HasConvert, T, DbType>;

}  // namespace impl

template <typename T, typename DbType>
T DoConvert(DbType&& from) {
  static_assert(
      // TODO : better wording
      impl::kHasConvert<T, DbType>,
      "There is no 'T Convert(From&&, storages::mysql::convert::To<T>)' in "
      "neither namespace of 'T' or `storages::mysql::convert`");

  return Convert(std::forward<DbType>(from), To<T>{});
}

}  // namespace storages::mysql::convert

USERVER_NAMESPACE_END
