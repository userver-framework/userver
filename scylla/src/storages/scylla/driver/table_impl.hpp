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
    operations::SelectOne::Row Execute(const operations::SelectOne&) override;
    operations::SelectMany::ResultSet Execute(const operations::SelectMany&) override;
    void Execute(const operations::DeleteOne&) override;
    void Execute(const operations::UpdateOne&) override;
    int64_t Execute(const operations::Count&) override;
    void Execute(const operations::InsertMany&) override;
    void Execute(const operations::Truncate&) override;

private:
    SessionImplPtr session_impl_;
};

}

USERVER_NAMESPACE_END
