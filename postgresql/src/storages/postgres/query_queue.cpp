#include <userver/storages/postgres/query_queue.hpp>

#include <fmt/format.h>

#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

struct QueryQueue::QueriesStorage final {
  struct QueryMeta final {
    std::string prepared_statement_name;
    ParamsHolder params;
    TimeoutDuration statement_timeout;

    QueryMeta(std::string prepared_statement_name, ParamsHolder&& params,
              TimeoutDuration statement_timeout)
        : prepared_statement_name{std::move(prepared_statement_name)},
          params{std::move(params)},
          statement_timeout{statement_timeout} {}
  };

  std::vector<QueryMeta> queries;
  std::vector<ResultSet> descriptions;
};

QueryQueue::QueryQueue(CommandControl default_cc, detail::ConnectionPtr&& conn)
    : default_cc_{default_cc}, conn_{std::move(conn)} {
  UINVARIANT(conn_->IsPipelineActive(),
             "QueryQueue usage requires pipelining to be enabled");

  UINVARIANT(conn_->ArePreparedStatementsEnabled(),
             "QueryQueue usage requires prepared statements to be enabled");
}

QueryQueue::QueryQueue(QueryQueue&&) noexcept = default;

QueryQueue& QueryQueue::operator=(QueryQueue&&) noexcept = default;

QueryQueue::~QueryQueue() = default;

void QueryQueue::Reserve(std::size_t size) {
  queries_storage_->queries.reserve(size);
  queries_storage_->descriptions.reserve(size);
}

std::vector<ResultSet> QueryQueue::Collect() {
  return QueryQueue::Collect(default_cc_.execute);
}

std::vector<ResultSet> QueryQueue::Collect(TimeoutDuration timeout) {
  ValidateUsage();

  tracing::Span collect_span{"query_queue_collect"};
  auto scope = collect_span.CreateScopeTime();

  const USERVER_NAMESPACE::utils::ScopeGuard reset_guard{[this] {
    [[maybe_unused]] const detail::ConnectionPtr tmp{std::move(conn_)};
  }};

  if (queries_storage_->queries.empty()) {
    return {};
  }

  UASSERT(queries_storage_->queries.size() ==
          queries_storage_->descriptions.size());

  for (std::size_t i = 0; i < queries_storage_->queries.size(); ++i) {
    const auto& meta = queries_storage_->queries[i];
    const auto& description = queries_storage_->descriptions[i];
    const CommandControl cc{
        timeout /* .execute, used for SetStatementTimeout deadline */,
        meta.statement_timeout /* .statement, used as expected */};
    conn_->AddIntoPipeline(cc, meta.prepared_statement_name,
                           meta.params.params_proxy, description, scope);
  }

  auto result = conn_->GatherPipeline(timeout, queries_storage_->descriptions);
  if (result.size() != queries_storage_->queries.size()) {
    throw RuntimeError{
        fmt::format("QueryQueue results count mismatch: expected {}, got {}",
                    queries_storage_->queries.size(), result.size())};
  }
  return result;
}

const UserTypes& QueryQueue::GetConnectionUserTypes() const {
  return conn_->GetUserTypes();
}

void QueryQueue::DoPush(CommandControl cc, const Query& query,
                        ParamsHolder&& params) {
  ValidateUsage();

  auto prepared_statement_meta =
      conn_->PrepareStatement(query, params.params_proxy, cc.execute);
  queries_storage_->queries.emplace_back(
      std::move(prepared_statement_meta.statement_name), std::move(params),
      cc.statement);
  queries_storage_->descriptions.emplace_back(
      std::move(prepared_statement_meta.description));
}

void QueryQueue::ValidateUsage() const {
  UINVARIANT(conn_, "The query queue is finalized and no longer usable.");
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
