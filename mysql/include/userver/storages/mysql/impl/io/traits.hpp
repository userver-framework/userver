#pragma once

#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

template <typename Container>
auto Inserter(Container& container) {
  return meta::Inserter(container);
}

template <typename Container>
inline constexpr bool kIsReservable = meta::kIsReservable<Container>;

template <typename Container>
inline constexpr bool kIsRange = meta::kIsRange<Container>;

template <typename Container>
inline constexpr bool kIsSizeable = meta::kIsSizable<Container>;

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
