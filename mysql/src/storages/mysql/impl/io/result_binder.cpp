#include <userver/storages/mysql/impl/io/result_binder.hpp>

#include <storages/mysql/impl/bindings/output_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

ResultBinder::ResultBinder(std::size_t size) : impl_{size} {}

ResultBinder::~ResultBinder() = default;

ResultBinder::ResultBinder(ResultBinder&& other) noexcept
    : impl_{std::move(other.impl_)} {}

impl::bindings::OutputBindings& ResultBinder::GetBinds() { return *impl_; }

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
