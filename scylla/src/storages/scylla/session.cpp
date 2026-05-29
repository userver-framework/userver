#include <userver/storages/scylla/session.hpp>

#include <utility>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/storages/scylla/session_config.hpp>
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
    utils::zstring_view hosts,
    const SessionConfig& session_config,
    dynamic_config::Source config_source,
    clients::dns::Resolver* dns_resolver,
    std::optional<SslSecrets> ssl_secrets
)
    : impl_(std::make_shared<impl::driver::DriverSessionImpl>(
          std::move(id),
          hosts,
          session_config,
          config_source,
          dns_resolver,
          std::move(ssl_secrets)
      ))
{}

Session::Session(Session&&) noexcept = default;
Session& Session::operator=(Session&&) noexcept = default;
Session::~Session() = default;

Table Session::GetTable(std::string table_name) const {
    return Table(std::make_shared<
                 impl::driver::DriverTableImpl>(impl_, impl_->DefaultDatabaseName(), std::move(table_name)));
}

void Session::DropKeyspace() { impl_->DropKeyspace(); }

void Session::Ping() { impl_->Ping(); }

void Session::SetContactPoints(utils::zstring_view contact_points) { impl_->SetConnectionString(contact_points); }

Rows Session::Execute(std::string query) { return impl_->ExecuteRaw(query, {}); }

Rows Session::Execute(std::string query, std::vector<Value> params) { return impl_->ExecuteRaw(query, params); }

PagedRows Session::ExecutePaged(
    std::string query,
    std::vector<Value> params,
    std::size_t page_size,
    std::string paging_state
) {
    return impl_->ExecuteRawPaged(query, params, page_size, paging_state);
}

void Session::ExecuteVoid(std::string query, std::vector<Value> params) { impl_->ExecuteRawVoid(query, params); }

Cursor Session::NewCursor(std::string query, std::vector<Value> params, std::size_t page_size) {
    return Cursor{impl_, std::move(query), std::move(params), page_size};
}

void DumpMetric(utils::statistics::Writer& writer, const Session& session) {
    UASSERT(session.impl_);
    writer = *session.impl_->GetStatistics().session;
}

Cursor::Cursor(
    std::shared_ptr<impl::SessionImpl> session_impl,
    std::string query,
    std::vector<Value> params,
    std::size_t page_size
)
    : session_impl_(std::move(session_impl)),
      query_(std::move(query)),
      params_(std::move(params)),
      page_size_(page_size == 0 ? 1000 : page_size)
{}

std::optional<Rows> Cursor::NextPage() {
    if (done_) {
        return std::nullopt;
    }

    auto page = session_impl_->ExecuteRawPaged(query_, params_, page_size_, paging_state_);

    paging_state_ = std::move(page.paging_state);
    if (!page.has_more_pages) {
        done_ = true;
    }
    return std::move(page.rows);
}

}  // namespace storages::scylla

USERVER_NAMESPACE_END
