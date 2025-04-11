#include <userver/storages/sqlite/impl/io/params_binder_base.hpp>

#include <userver/storages/sqlite/impl/statement.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

ParamsBinderBase::ParamsBinderBase(const std::string& query, infra::ConnectionPtr& conn)
    : binds_impl_{conn->PrepareStatement(query)} {}

ParamsBinderBase::ParamsBinderBase(ParamsBinderBase&& other) noexcept : binds_impl_{std::move(other.binds_impl_)} {}

InputBindingsFwd& ParamsBinderBase::GetBinds() { return *binds_impl_; }

InputBindingsPimpl ParamsBinderBase::GetBindsPtr() { return binds_impl_; }

ParamsBinderBase::~ParamsBinderBase() = default;

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
