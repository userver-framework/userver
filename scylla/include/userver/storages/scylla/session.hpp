#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/dynamic_config/fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/zstring_view.hpp>

#include <userver/storages/scylla/row.hpp>
#include <userver/storages/scylla/session_config.hpp>
#include <userver/storages/scylla/table.hpp>
#include <userver/storages/scylla/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

namespace impl {
class SessionImpl;
}

class Cursor {
public:
    std::optional<Rows> NextPage();

    bool Done() const noexcept { return done_; }
    std::size_t PageSize() const noexcept { return page_size_; }

private:
    friend class Session;

    Cursor(
        std::shared_ptr<impl::SessionImpl> session_impl,
        std::string query,
        std::vector<Value> params,
        std::size_t page_size
    );

    std::shared_ptr<impl::SessionImpl> session_impl_;
    std::string query_;
    std::vector<Value> params_;
    std::size_t page_size_;
    std::string paging_state_;
    bool done_{false};
};

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

    Rows Execute(std::string query);
    Rows Execute(std::string query, std::vector<Value> params);

    template <typename... Args>
    Rows Execute(std::string query, Args&&... args) {
        return Execute(std::move(query), std::vector<Value>{Value{std::forward<Args>(args)}...});
    }

    PagedRows ExecutePaged(
        std::string query,
        std::vector<Value> params,
        std::size_t page_size,
        std::string paging_state = {}
    );

    void ExecuteVoid(std::string query, std::vector<Value> params = {});

    Cursor NewCursor(std::string query, std::vector<Value> params = {}, std::size_t page_size = 1000);

    explicit Session(
        std::string id,
        utils::zstring_view hosts,
        const SessionConfig& session_config,
        dynamic_config::Source config_source,
        clients::dns::Resolver* dns_resolver,
        std::optional<SslSecrets> ssl_secrets = std::nullopt
    );

    friend void DumpMetric(utils::statistics::Writer& writer, const Session& session);

    void SetContactPoints(utils::zstring_view contact_points);

private:
    std::shared_ptr<impl::SessionImpl> impl_;
};

using SessionPtr = std::shared_ptr<Session>;

}  // namespace storages::scylla

USERVER_NAMESPACE_END
