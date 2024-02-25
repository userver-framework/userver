#pragma once

/// @file userver/storages/postgres/query_queue.hpp
/// @brief An utility to execute multiple queries in a single network
/// round-trip.

#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/query.hpp>
#include <userver/storages/postgres/result_set.hpp>

#include <userver/storages/postgres/detail/connection_ptr.hpp>
#include <userver/storages/postgres/detail/query_parameters.hpp>

#include <userver/utils/any_movable.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @brief A container to enqueue queries in FIFO order and execute them all
/// within a single network round-trip.
///
/// Acquired from storages::postgres::Cluster, one is expected to `Push`
/// some queries into the queue and then `Collect` them into vector of results.
///
/// From a client point of view `Collect` is transactional: either all the
/// queries succeed or `Collect` rethrows the first error encountered. However,
/// this is *NOT* the case for the server: server treats all the provided
/// queries independently and is likely to  execute subsequent queries even
/// after prior failures.
///
/// @warning If an explicit transaction ("BEGIN") or a modifying query is added
/// into the queue the behavior is unspecified. *Don't do that*.
///
/// @note Queries may or may not be sent to the server prior to `Collect` call.
///
/// @note Requires both pipelining and prepared statements to be enabled in the
/// driver, UINVARIANTS (that is, throws in release builds and aborts in debug
/// ones) that on construction.
class QueryQueue final {
 public:
  QueryQueue(CommandControl default_cc, detail::ConnectionPtr&& conn);

  QueryQueue(QueryQueue&&) noexcept;
  QueryQueue& operator=(QueryQueue&&) noexcept;

  QueryQueue(const QueryQueue&) = delete;
  QueryQueue& operator=(const QueryQueue&) = delete;

  ~QueryQueue();

  /// Reserve internal storage to hold this amount of queries.
  void Reserve(std::size_t size);

  /// Add a query into the queue with specified command-control.
  /// CommandControl is used as following: 'execute' is used as a timeout for
  /// preparing the statement (if needed), 'statement' is used as a statement
  /// timeout for later execution.
  template <typename... Args>
  void Push(CommandControl cc, const Query& query, const Args&... args);

  /// Add a query into the queue with default command-control.
  template <typename... Args>
  void Push(const Query& query, const Args&... args);

  /// Collect results of all the queued queries, with specified timeout.
  /// Either returns a vector of N `ResultSet`s, where N is the number of
  /// queries enqueued, or rethrow the first error encountered, be that a query
  /// execution error or a timeout.
  [[nodiscard]] std::vector<ResultSet> Collect(TimeoutDuration timeout);

  /// Collect results of all the queued queries, with default timeout.
  /// Either returns a vector of N `ResultSet`s, where N is the number of
  /// queries enqueued, or rethrow the first error encountered, be that a query
  /// execution error or a timeout.
  [[nodiscard]] std::vector<ResultSet> Collect();

 private:
  struct ParamsHolder final {
    // We only need to know what's here at construction (and at construction we
    // do know), after that we just need to preserve lifetime.
    USERVER_NAMESPACE::utils::AnyMovable any_params{};
    // This here knows what's there actually is in params.
    detail::QueryParameters params_proxy;
  };

  const UserTypes& GetConnectionUserTypes() const;

  void DoPush(CommandControl cc, const Query& query, ParamsHolder&& params);

  void ValidateUsage() const;

  CommandControl default_cc_;
  detail::ConnectionPtr conn_;

  struct QueriesStorage;
  USERVER_NAMESPACE::utils::FastPimpl<QueriesStorage, 48, 8> queries_storage_;
};

template <typename... Args>
void QueryQueue::Push(CommandControl cc, const Query& query,
                      const Args&... args) {
  ParamsHolder holder{};
  auto& params = holder.any_params
                     .Emplace<detail::StaticQueryParameters<sizeof...(args)>>();
  params.Write(GetConnectionUserTypes(), args...);
  holder.params_proxy = detail::QueryParameters{params};
  DoPush(cc, query, std::move(holder));
}

template <typename... Args>
void QueryQueue::Push(const Query& query, const Args&... args) {
  Push(default_cc_, query, args...);
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
