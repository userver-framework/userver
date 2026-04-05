#pragma once

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

private:
    SessionImplPtr session_impl_;
};
}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END