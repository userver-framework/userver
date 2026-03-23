#include "session_impl.hpp"

#include <stdexcept>
#include <string_view>

#include <cassandra.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

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

DriverSessionImpl::ConnPtr DriverSessionImpl::Create() {
    LOG_DEBUG() << "Creating scylla connection";

    CassCluster* cluster = cass_cluster_new();
    cass_cluster_set_contact_points(cluster, hosts_.c_str());

    CassSession* session = cass_session_new();
    CassFuture* future = cass_session_connect(session, cluster);
    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        const char* message = nullptr;
        size_t message_length = 0;
        cass_future_error_message(future, &message, &message_length);
        LOG_ERROR() << "Scylla connect failed: " << std::string_view(message, message_length);
        cass_future_free(future);
        cass_session_free(session);
        cass_cluster_free(cluster);
        throw std::runtime_error("Failed to connect to ScyllaDB");
    }
    cass_future_free(future);

    return std::make_shared<Connection>(session, cluster);
}

const std::string& DriverSessionImpl::DefaultDatabaseName() const { return default_keyspace_; }

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
