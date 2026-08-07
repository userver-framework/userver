#include "connection.hpp"

#include <userver/logging/log.hpp>

#include <ydb/impl/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

NYdb::NTable::TClientSettings MakeTableSettings(const TableSettings& settings) {
    NYdb::NTable::TSessionPoolSettings session_pool_settings;
    session_pool_settings.MaxActiveSessions(settings.max_pool_size)
        .MinPoolSize(settings.min_pool_size)
        .RetryLimit(settings.get_session_retry_limit);

    NYdb::NTable::TClientSettings table_settings;
    table_settings.SessionPoolSettings(session_pool_settings);
    return table_settings;
}

}  // namespace

NYdb::NQuery::TClientSettings MakeQuerySettings(const TableSettings& settings) {
    NYdb::NQuery::TSessionPoolSettings session_pool_settings;
    session_pool_settings.MaxActiveSessions(settings.max_pool_size)
        .MinPoolSize(settings.min_pool_size)
        .UseDeferredSessionCreation(settings.use_deferred_session_creation);

    NYdb::NQuery::TClientSettings query_settings;
    query_settings.SessionPoolSettings(session_pool_settings);
    return query_settings;
}

Connection::Connection(std::shared_ptr<Driver> from_driver, const TableSettings& settings)
    : driver(std::move(from_driver)),
      scheme_client(driver->GetNativeDriver(), MakeTableSettings(settings)),
      table_client(driver->GetNativeDriver(), MakeTableSettings(settings)),
      query_client(driver->GetNativeDriver(), MakeQuerySettings(settings))
{}

Connection::~Connection() {
    // Only NYdb::NTable::TTableClient (legacy Table API) exposes an explicit
    // async Stop() that drains its session pool and must be awaited before the
    // driver is destroyed (using the client after Stop() is UB). The newer
    // NYdb::NQuery::TQueryClient and the scheme client have no Stop() and
    // release their pools via RAII on destruction, so nothing to await there.
    try {
        GetFutureValue(table_client.Stop());
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while stopping TTableClient: " << ex;
    }
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
