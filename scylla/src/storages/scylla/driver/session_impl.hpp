#pragma once

#include <memory>
#include <optional>
#include <string>

#include <cassandra.h>

#include <storages/scylla/driver/prepared_cache.hpp>
#include <storages/scylla/session_impl.hpp>
#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/dynamic_config/fwd.hpp>
#include <userver/storages/scylla/session_config.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

class DriverSessionImpl : public SessionImpl {
public:
    DriverSessionImpl(
        std::string id,
        const std::string& hosts,
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
        Connection(CassSession* session, CassCluster* cluster) : session_(session), cluster_(cluster) {
            UASSERT(session_);
            UASSERT(cluster_);
        }

        CassSession* GetSession() { return session_.get(); }

    private:
        std::unique_ptr<CassSession, SessionDeleter> session_;
        std::unique_ptr<CassCluster, ClusterDeleter> cluster_;
    };

    const std::string& DefaultDatabaseName() const override;

    CassSession* GetNativeSession() { return connection_->GetSession(); }

    const SessionConfig& GetSessionConfig() const noexcept { return session_config_; }

    PreparedStatementCache& GetPreparedCache() noexcept { return prepared_cache_; }

    void Ping() override;

    void DropKeyspace() override;

    void SetConnectionString(const std::string& connection_string) override;

    Rows ExecuteRaw(const std::string& query, const std::vector<Value>& params) override;
    PagedRows ExecuteRawPaged(
        const std::string& query,
        const std::vector<Value>& params,
        std::size_t page_size,
        const std::string& paging_state) override;
    void ExecuteRawVoid(const std::string& query, const std::vector<Value>& params) override;

    using ConnPtr = std::shared_ptr<Connection>;

private:
    ConnPtr Create();

    SessionConfig session_config_;
    std::optional<storages::scylla::SslSecrets> ssl_secrets_;
    std::string hosts_;
    std::string default_keyspace_;
    ConnPtr connection_;
    PreparedStatementCache prepared_cache_;
};

}

USERVER_NAMESPACE_END
