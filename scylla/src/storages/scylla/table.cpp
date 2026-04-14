#include <userver/storages/scylla/table.hpp>

#include <userver/storages/scylla/table_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

Table::Table(std::shared_ptr<impl::TableImpl> impl) : impl_(impl) {}

const std::string& Table::GetTableName() const { return impl_->GetTableName(); }

void Table::Execute(const operations::InsertOne& insert_op) { impl_->Execute(insert_op); }

operations::SelectOne::Row Table::Execute(const operations::SelectOne& select_op) {
    return impl_->Execute(select_op);
}

operations::SelectMany::ResultSet Table::Execute(const operations::SelectMany& select_op) {
    return impl_->Execute(select_op);
}

void Table::Execute(const operations::DeleteOne& delete_op) { impl_->Execute(delete_op); }

void Table::Execute(const operations::UpdateOne& update_op) { impl_->Execute(update_op); }

int64_t Table::Execute(const operations::Count& count_op) { return impl_->Execute(count_op); }

void Table::Execute(const operations::InsertMany& insert_op) { impl_->Execute(insert_op); }

void Table::Execute(const operations::Truncate& truncate_op) { impl_->Execute(truncate_op); }

}

USERVER_NAMESPACE_END
