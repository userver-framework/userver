#pragma once

#include <userver/storages/mysql/impl/io/common_binders.hpp>
#include <userver/storages/mysql/impl/io/decimal_binder.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

template <typename T>
void BindInput(mysql::impl::InputBindingsFwd& binds, std::size_t pos,
               const T& field) {
  using SteadyClock = std::chrono::steady_clock;
  static_assert(!std::is_same_v<SteadyClock::time_point, T> &&
                    !std::is_same_v<std::optional<SteadyClock::time_point>, T>,
                "Don't store steady_clock times in the database, use "
                "system_clock instead");

  storages::mysql::impl::io::FreestandingBind(binds, pos,
                                              ExplicitCRef<T>{field});
}

template <typename T>
void BindOutput(mysql::impl::OutputBindingsFwd& binds, std::size_t pos,
                T& field) {
  static_assert(
      !std::is_same_v<std::string_view, T> &&
          !std::is_same_v<std::optional<std::string_view>, T>,
      "Don't use std::string_view in output params, since it's not-owning");

  storages::mysql::impl::io::FreestandingBind(binds, pos,
                                              ExplicitRef<T>{field});
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
