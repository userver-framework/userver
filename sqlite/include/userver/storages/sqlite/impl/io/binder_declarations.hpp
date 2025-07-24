#pragma once

#include <userver/storages/sqlite/impl/binder_fwd.hpp>
#include <userver/storages/sqlite/impl/io/common_binders.hpp>
#include <userver/storages/sqlite/impl/io/decimal_binders.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

template <typename T>
void BindInput(sqlite::impl::InputBindingsFwd& binds, std::size_t pos, const T& field) {
    storages::sqlite::impl::io::FreestandingBind(binds, pos, ExplicitCRef<T>{field});
}

template <typename T>
void BindOutput(sqlite::impl::OutputBindingsFwd& binds, std::size_t pos, T& field) {
    storages::sqlite::impl::io::FreestandingBind(binds, pos, ExplicitRef<T>{field});
}

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
