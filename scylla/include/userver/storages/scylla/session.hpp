#pragma once

#include <memory>
#include <optional>
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

class Session {
public:
    Session(Session&&) noexcept;
    Session& operator=(Session&&) noexcept;
    ~Session();

    bool HasTable(utils::zstring_view name) const;

    Table GetTable(std::string table_name) const;

    void DropKeyspace();

    std::vector<std::string> ListTableNames() const;

    void Ping();

    explicit Session(
        std::string id,
        const std::string& hosts,
        const SessionConfig& session_config,
        dynamic_config::Source config_source,
        clients::dns::Resolver* dns_resolver,
        std::optional<SslSecrets> ssl_secrets = std::nullopt
    );

    friend void DumpMetric(utils::statistics::Writer& writer, const Session& session);

    void SetSessionSettings(const SessionSettings& session_settings);

    void SetContactPoints(const std::string& contact_points);

private:
    std::shared_ptr<impl::SessionImpl> impl_;
};

using SessionPtr = std::shared_ptr<Session>;

}

USERVER_NAMESPACE_END
