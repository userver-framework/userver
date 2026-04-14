#include <userver/dynamic_config/source.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/storages/scylla/session_config.hpp>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <storages/scylla/driver/session_impl.hpp>
#include <storages/scylla/driver/table_impl.hpp>
#include <storages/scylla/scylla_secdist.hpp>
#include <storages/scylla/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

Session::Session(
    std::string id,
    const std::string& hosts,
    const SessionConfig& session_config,
    dynamic_config::Source config_source,
    clients::dns::Resolver* dns_resolver,
    std::optional<SslSecrets> ssl_secrets
)
    : impl_(
          std::make_shared<impl::driver::DriverSessionImpl>(
              std::move(id), hosts, session_config, config_source, dns_resolver, std::move(ssl_secrets))
      ) {}

Session::~Session() = default;

Table Session::GetTable(std::string table_name) const {
    return Table(
        std::make_shared<impl::driver::DriverTableImpl>(impl_, impl_->DefaultDatabaseName(), std::move(table_name))
    );
}

void Session::DropKeyspace() { impl_->DropKeyspace(); }

void Session::Ping() { impl_->Ping(); }

void Session::SetContactPoints(const std::string& contact_points) {
    impl_->SetConnectionString(contact_points);
}

void DumpMetric(utils::statistics::Writer& writer, const Session& session) {
    UASSERT(session.impl_);
    writer = *session.impl_->GetStatistics().session;
}

}

USERVER_NAMESPACE_END
