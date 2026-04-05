#include <userver/storages/scylla/table.hpp>

#include <userver/storages/scylla/table_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

Table::Table(std::shared_ptr<impl::TableImpl> impl) : impl_(impl) {}

const std::string& Table::GetTableName() const { return impl_->GetTableName(); }

// TODO: WriteResult instead of void
void Table::Execute(const operations::InsertOne& insert_op) { return impl_->Execute(insert_op); }

operations::SelectOne::Row Table::Execute(const operations::SelectOne& select_op) { return impl_->Execute(select_op); }
}  // namespace storages::scylla

USERVER_NAMESPACE_END