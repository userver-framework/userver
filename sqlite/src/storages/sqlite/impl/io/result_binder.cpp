#include <userver/storages/sqlite/impl/io/result_binder.hpp>

#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/impl/statement_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

ResultBinder::ResultBinder(impl::ResultWrapper& result_wrapper) : impl_{result_wrapper.GetStatement()} {}

ResultBinder::~ResultBinder() = default;

ResultBinder::ResultBinder(ResultBinder&& other) noexcept = default;

OutputBindingsFwd& ResultBinder::GetBinds() { return *impl_; }

int ResultBinder::ColumnCount() { return impl_->ColumnCount(); }

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
