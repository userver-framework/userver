#include <userver/storages/mysql/statement_result_set.hpp>

#include <optional>

#include <userver/tracing/span.hpp>

#include <storages/mysql/impl/statement.hpp>
#include <storages/mysql/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

struct StatementResultSet::Impl final {
  Impl(impl::StatementFetcher&& fetcher, tracing::Span&& span)
      : fetcher{std::move(fetcher)}, span{std::move(span)} {}

  Impl(infra::ConnectionPtr&& connection, impl::StatementFetcher&& fetcher,
       tracing::Span&& span)
      : owned_connection{std::move(connection)},
        fetcher{std::move(fetcher)},
        span{std::move(span)} {}

  std::optional<infra::ConnectionPtr> owned_connection;
  impl::StatementFetcher fetcher;
  tracing::Span span;
};

StatementResultSet::StatementResultSet(impl::StatementFetcher&& fetcher,
                                       tracing::Span&& span)
    : impl_{std::move(fetcher), std::move(span)} {}

StatementResultSet::StatementResultSet(infra::ConnectionPtr&& connection,
                                       impl::StatementFetcher&& fetcher,
                                       tracing::Span&& span)
    : impl_{std::move(connection), std::move(fetcher), std::move(span)} {}

StatementResultSet::~StatementResultSet() = default;

StatementResultSet::StatementResultSet(StatementResultSet&& other) noexcept =
    default;

ExecutionResult StatementResultSet::AsExecutionResult() && {
  const auto rows_affected = impl_->fetcher.RowsAffected();
  const auto last_insert_id = impl_->fetcher.LastInsertId();

  ExecutionResult result{};
  result.rows_affected = rows_affected;
  result.last_insert_id = last_insert_id;
  return result;
}

bool StatementResultSet::FetchResult(impl::io::ExtractorBase& extractor) {
  return impl_->fetcher.FetchResult(extractor);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
