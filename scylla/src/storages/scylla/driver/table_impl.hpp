#pragma once

#include <cstdint>
#include <string>

#include <storages/scylla/session_impl.hpp>
#include <userver/storages/scylla/table_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

class DriverTableImpl : public TableImpl {
public:
    DriverTableImpl(SessionImplPtr session_impl, std::string keyspace_name, std::string table_name);

    void Execute(const operations::InsertOne&) override;
    Row Execute(const operations::SelectOne&) override;
    Rows Execute(const operations::SelectMany&) override;
    void Execute(const operations::DeleteOne&) override;
    void Execute(const operations::UpdateOne&) override;
    std::int64_t Execute(const operations::Count&) override;
    void Execute(const operations::InsertMany&) override;
    void Execute(const operations::Truncate&) override;

    PagedRows ExecutePaged(const operations::SelectMany&, std::string paging_state) override;

    operations::LwtResult ExecuteLwt(const operations::InsertOne&) override;
    operations::LwtResult ExecuteLwt(const operations::UpdateOne&) override;
    operations::LwtResult ExecuteLwt(const operations::DeleteOne&) override;

private:
    SessionImplPtr session_impl_;
};

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
