#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/dynamic_config/fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/zstring_view.hpp>

#include <userver/storages/scylla/session_config.hpp>
#include <userver/storages/scylla/table.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

namespace impl {
class SessionImpl;
}

/// @brief ScyllaDB client session.
///
/// Use constructor only for tests, in production the session should be
/// retrieved from @ref userver_components "the components" via
/// components::Scylla::GetSession().
class Session {
public:
    Session(Session&&) noexcept;
    Session& operator=(Session&&) noexcept;
    ~Session();

    /// Checks whether a table exists in the keyspace
    bool HasTable(utils::zstring_view name) const;

    Table GetTable(std::string table_name) const;

    // PreparedStatement Prepare(std::string query) const;

    /// Execute a CQL query
    // ResultSet Execute(const Statement& statement) const;

    /// Drops the associated keyspace if it exists
    void DropKeyspace();

    /// Get a list of all table names in the associated keyspace
    std::vector<std::string> ListTableNames() const;

    /// @throws storages::scylla::ScyllaException if failed to connect
    void Ping();

    /// @cond
    explicit Session(
        std::string id,
        const std::string& hosts,
        const SessionConfig& session_config,
        dynamic_config::Source config_source,
        // ??
        // const std::string& contact_points,
        clients::dns::Resolver* dns_resolver
    );

    friend void DumpMetric(utils::statistics::Writer& writer, const Session& session);

    void SetSessionSettings(const SessionSettings& session_settings);

    void SetContactPoints(const std::string& contact_points);
    /// @endcond

private:
    std::shared_ptr<impl::SessionImpl> impl_;
};

using SessionPtr = std::shared_ptr<Session>;

}  // namespace storages::scylla

USERVER_NAMESPACE_END