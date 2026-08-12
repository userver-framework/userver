#include <userver/storages/scylla/table.hpp>

#include <utility>

#include <userver/storages/scylla/table_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

Table::Table(std::shared_ptr<impl::TableImpl> impl)
    : impl_(std::move(impl))
{}

const std::string& Table::GetTableName() const { return impl_->GetTableName(); }

void Table::Execute(const operations::InsertOne& op) { impl_->Execute(op); }
Row Table::Execute(const operations::SelectOne& op) { return impl_->Execute(op); }
Rows Table::Execute(const operations::SelectMany& op) { return impl_->Execute(op); }
void Table::Execute(const operations::DeleteOne& op) { impl_->Execute(op); }
void Table::Execute(const operations::UpdateOne& op) { impl_->Execute(op); }
std::int64_t Table::Execute(const operations::Count& op) { return impl_->Execute(op); }
void Table::Execute(const operations::InsertMany& op) { impl_->Execute(op); }
void Table::Execute(const operations::Truncate& op) { impl_->Execute(op); }

PagedRows Table::ExecutePaged(const operations::SelectMany& op, std::string paging_state) {
    return impl_->ExecutePaged(op, std::move(paging_state));
}

operations::LwtResult Table::ExecuteLwt(const operations::InsertOne& op) { return impl_->ExecuteLwt(op); }
operations::LwtResult Table::ExecuteLwt(const operations::UpdateOne& op) { return impl_->ExecuteLwt(op); }
operations::LwtResult Table::ExecuteLwt(const operations::DeleteOne& op) { return impl_->ExecuteLwt(op); }

}  // namespace storages::scylla

USERVER_NAMESPACE_END
