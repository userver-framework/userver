#include <userver/dynamic_config/source.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/storages/scylla/session_config.hpp>

#include <userver/clients/dns/resolver_utils.hpp>

#include <storages/scylla/driver/session_impl.hpp>
#include <storages/scylla/driver/table_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

Session::Session(
    std::string id,
    const std::string& hosts,
    const SessionConfig& session_config,
    dynamic_config::Source config_source,
    clients::dns::Resolver* dns_resolver
)
    : impl_(
          std::make_shared<
              impl::driver::DriverSessionImpl>(std::move(id), hosts, session_config, config_source, dns_resolver)
      ) {}

Session::~Session() = default;

Table Session::GetTable(std::string table_name) const {
    return Table(
        std::make_shared<impl::driver::DriverTableImpl>(impl_, impl_->DefaultDatabaseName(), std::move(table_name))
    );
}

}  // namespace storages::scylla

USERVER_NAMESPACE_END
