#include <userver/storages/mysql/statement_result_set.hpp>

#include <optional>

#include <storages/mysql/impl/mysql_statement.hpp>
#include <storages/mysql/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

struct StatementResultSet::Impl final {
  Impl(impl::MySQLStatementFetcher&& fetcher) : fetcher{std::move(fetcher)} {}

  Impl(infra::ConnectionPtr&& connection, impl::MySQLStatementFetcher&& fetcher)
      : owned_connection{std::move(connection)}, fetcher{std::move(fetcher)} {}

  std::optional<infra::ConnectionPtr> owned_connection;
  impl::MySQLStatementFetcher fetcher;
};

StatementResultSet::StatementResultSet(impl::MySQLStatementFetcher&& fetcher)
    : impl_{std::move(fetcher)} {}

StatementResultSet::StatementResultSet(infra::ConnectionPtr&& connection,
                                       impl::MySQLStatementFetcher&& fetcher)
    : impl_{std::move(connection), std::move(fetcher)} {}

StatementResultSet::~StatementResultSet() = default;

StatementResultSet::StatementResultSet(StatementResultSet&& other) noexcept =
    default;

bool StatementResultSet::FetchResult(impl::io::ExtractorBase& extractor) {
  return impl_->fetcher.FetchResult(extractor);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
