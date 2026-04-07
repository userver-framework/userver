#include "session_impl.hpp"

#include <memory>

#include <cassandra.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/logging/log.hpp>

#include <storages/scylla/driver/cass_wrappers.hpp>
#include <storages/scylla/driver/scylla_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

namespace {

constexpr const char* kPingQuery = "SELECT release_version FROM system.local";

}  // namespace

DriverSessionImpl::DriverSessionImpl(
    std::string id,
    const std::string& hosts,
    const SessionConfig& session_config,
    dynamic_config::Source config_source,
    clients::dns::Resolver* dns_resolver
)
    : SessionImpl(std::move(id), session_config, config_source),
      hosts_(hosts),
      default_keyspace_(session_config.default_keyspace),
      connection_(Create()) {}


void DriverSessionImpl::SetConnectionString(const std::string& connection_string) {

}

DriverSessionImpl::ConnPtr DriverSessionImpl::Create() {
    LOG_DEBUG() << "Creating scylla connection";

    CassClusterPtr cluster(cass_cluster_new());
    cass_cluster_set_contact_points(cluster.get(), hosts_.c_str());

    CassSessionPtr session(cass_session_new());

    CassFuturePtr connect_future(
        cass_session_connect(session.get(), cluster.get()));
    cass_future_wait(connect_future.get());
    CheckFuture(connect_future.get(), "connect");

    CassStatementPtr ping_stmt(cass_statement_new(kPingQuery, 0));
    CassFuturePtr ping_future(
        cass_session_execute(session.get(), ping_stmt.get()));
    cass_future_wait(ping_future.get());
    CheckFuture(ping_future.get(), "ping");

    return std::make_shared<Connection>(session.release(), cluster.release());
}

const std::string& DriverSessionImpl::DefaultDatabaseName() const { return default_keyspace_; }

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
