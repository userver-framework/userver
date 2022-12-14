#include <userver/storages/mysql/statement_result_set.hpp>

#include <storages/mysql/impl/mysql_statement.hpp>
#include <storages/mysql/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

struct StatementResultSet::Impl final {
  Impl(infra::ConnectionPtr&& connection, impl::MySQLStatementFetcher&& fetcher)
      : connection{std::move(connection)}, fetcher{std::move(fetcher)} {}

  infra::ConnectionPtr connection;
  impl::MySQLStatementFetcher fetcher;
};

StatementResultSet::StatementResultSet(infra::ConnectionPtr&& connection,
                                       impl::MySQLStatementFetcher&& fetcher)
    : impl_{std::move(connection), std::move(fetcher)} {}

StatementResultSet::~StatementResultSet() = default;

StatementResultSet::StatementResultSet(StatementResultSet&& other) noexcept =
    default;

bool StatementResultSet::FetchResult(io::ExtractorBase& extractor) {
  return impl_->fetcher.FetchResult(extractor);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
