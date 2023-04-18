#include <userver/storages/mysql/impl/io/params_binder_base.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

ParamsBinderBase::ParamsBinderBase(std::size_t size) : binds_impl_{size} {}

ParamsBinderBase::ParamsBinderBase(ParamsBinderBase&& other) noexcept
    : binds_impl_{std::move(other.binds_impl_)} {}

InputBindingsFwd& ParamsBinderBase::GetBinds() { return *binds_impl_; }

ParamsBinderBase::~ParamsBinderBase() = default;

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
