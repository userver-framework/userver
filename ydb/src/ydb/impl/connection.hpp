#pragma once

#include <memory>

#include <ydb-cpp-sdk/client/query/client.h>
#include <ydb-cpp-sdk/client/query/query.h>
#include <ydb-cpp-sdk/client/table/table.h>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

NYdb::NQuery::TClientSettings MakeQuerySettings(const TableSettings& settings);

struct Connection final {
    std::shared_ptr<Driver> driver;
    NYdb::NScheme::TSchemeClient scheme_client;
    NYdb::NTable::TTableClient table_client;
    NYdb::NQuery::TQueryClient query_client;

    Connection(std::shared_ptr<Driver> driver, const TableSettings& settings);

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;

    ~Connection();
};

}  // namespace ydb::impl

USERVER_NAMESPACE_END
