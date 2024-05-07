#pragma once

#include <string>

#include <userver/tracing/span.hpp>

#include <userver/ydb/builder.hpp>
#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/impl/stats_scope.hpp>
#include <userver/ydb/query.hpp>
#include <userver/ydb/response.hpp>
#include <userver/ydb/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

/// @brief YDB Transaction
///
/// https://ydb.tech/docs/en/concepts/transactions
class Transaction final {
 public:
  Transaction(Transaction&&) noexcept = default;
  Transaction(const Transaction&) = delete;
  Transaction& operator=(Transaction&&) = delete;
  Transaction& operator=(const Transaction&) = delete;
  ~Transaction();

  /// Execute a single data query as a part of the transaction. Query parameters
  /// are passed in `Args` as "string key - value" pairs:
  ///
  /// @code
  /// client.ExecuteDataQuery(query, "name1", value1, "name2", value2, ...);
  /// @endcode
  ///
  /// Use ydb::PreparedArgsBuilder for storing a generic buffer of query params
  /// if needed.
  ///
  /// @{
  template <typename... Args>
  ExecuteResponse Execute(const Query& query, Args&&... args);

  template <typename... Args>
  ExecuteResponse Execute(OperationSettings settings, const Query& query,
                          Args&&... args);

  ExecuteResponse Execute(OperationSettings settings, const Query& query,
                          PreparedArgsBuilder&& builder);

  ExecuteResponse Execute(QuerySettings query_settings,
                          OperationSettings settings, const Query& query,
                          PreparedArgsBuilder&& builder);
  /// @}

  /// Commit the transaction. The options that are missing in `settings` are
  /// taken from the static config or driver defaults. `settings` can be
  /// overridden by dynamic config's options for `Commit` "query".
  void Commit(OperationSettings settings = {});

  /// Rollback the transaction. The operation settings are taken from `Begin`
  /// settings.
  void Rollback();

  PreparedArgsBuilder GetBuilder() const;

  /// @cond
  // For internal use only.
  Transaction(TableClient& table_client, NYdb::NTable::TTransaction ydb_tx,
              std::string name, OperationSettings&& rollback_settings) noexcept;
  /// @endcond

 private:
  void MarkError() noexcept;
  auto ErrorGuard();

  void EnsureActive() const;

  TableClient& table_client_;
  std::string name_;
  impl::StatsScope stats_scope_;
  tracing::Span span_;
  NYdb::NTable::TTransaction ydb_tx_;
  OperationSettings rollback_settings_;
  bool is_active_{true};
};

template <typename... Args>
ExecuteResponse Transaction::Execute(const Query& query, Args&&... args) {
  auto builder = GetBuilder();
  builder.AddParams(std::forward<Args>(args)...);
  return Execute(OperationSettings{}, query, std::move(builder));
}

template <typename... Args>
ExecuteResponse Transaction::Execute(OperationSettings settings,
                                     const Query& query, Args&&... args) {
  auto builder = GetBuilder();
  builder.AddParams(std::forward<Args>(args)...);
  return Execute(std::move(settings), query, std::move(builder));
}

}  // namespace ydb

USERVER_NAMESPACE_END
