#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <cassandra.h>

#include <storages/scylla/driver/prepared_cache.hpp>
#include <storages/scylla/session_impl.hpp>
#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/dynamic_config/fwd.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/storages/scylla/session_config.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

class DriverSessionImpl : public SessionImpl {
public:
    DriverSessionImpl(
        std::string id,
        utils::zstring_view hosts,
        const SessionConfig& session_config,
        dynamic_config::Source config_source,
        clients::dns::Resolver* dns_resolver,
        std::optional<storages::scylla::SslSecrets> ssl_secrets = std::nullopt
    );

    struct SessionDeleter {
        void operator()(CassSession* s) const noexcept {
            if (s) {
                cass_session_free(s);
            }
        }
    };

    struct ClusterDeleter {
        void operator()(CassCluster* c) const noexcept {
            if (c) {
                cass_cluster_free(c);
            }
        }
    };

    class Connection final {
    public:
        Connection(CassSession* session, CassCluster* cluster)
            : session_(session),
              cluster_(cluster)
        {
            UASSERT(session_);
            UASSERT(cluster_);
        }

        CassSession* GetSession() noexcept { return session_.get(); }

        PreparedStatementCache& GetPreparedCache() noexcept { return prepared_cache_; }

    private:
        std::unique_ptr<CassSession, SessionDeleter> session_;
        std::unique_ptr<CassCluster, ClusterDeleter> cluster_;
        PreparedStatementCache prepared_cache_;
    };

    using ConnPtr = std::shared_ptr<Connection>;

    const std::string& DefaultDatabaseName() const override;

    ConnPtr GetActiveConnection() const;

    const SessionConfig& GetSessionConfig() const noexcept { return session_config_; }

    void Ping() override;

    void DropKeyspace() override;

    void SetConnectionString(utils::zstring_view connection_string) override;

    Rows ExecuteRaw(std::string_view query, const std::vector<Value>& params) override;
    PagedRows ExecuteRawPaged(
        std::string_view query,
        const std::vector<Value>& params,
        std::size_t page_size,
        std::string_view paging_state
    ) override;
    void ExecuteRawVoid(std::string_view query, const std::vector<Value>& params) override;

private:
    ConnPtr Create(utils::zstring_view hosts);

    const SessionConfig session_config_;
    const std::optional<storages::scylla::SslSecrets> ssl_secrets_;
    const std::string default_keyspace_;

    mutable engine::SharedMutex conn_mutex_;
    std::string hosts_;
    ConnPtr connection_;
};

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
