#pragma once

#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::convert {

template <typename T>
struct To final {};

namespace impl {

template <typename T, typename MapFrom>
using HasConvert =
    decltype(MySQLConvert(std::declval<MapFrom&&>(), convert::To<T>{}));

template <typename T, typename MapFrom>
inline constexpr bool kHasConvert = meta::kIsDetected<HasConvert, T, MapFrom>;

}  // namespace impl

template <typename T, typename MapFrom>
T DoConvert(MapFrom&& from) {
  static_assert(
      // TODO : better wording
      impl::kHasConvert<T, MapFrom>,
      "There is no MySQLConvert(From&&, storages::mysql::convert::To<T>) in "
      "namespace of `storages::mysql::convert`");

  return MySQLConvert(std::forward<MapFrom>(from), To<T>{});
}

}  // namespace storages::mysql::convert

USERVER_NAMESPACE_END
