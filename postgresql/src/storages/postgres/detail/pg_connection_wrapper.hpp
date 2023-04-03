#pragma once

#include <chrono>
#include <string_view>

#include <libpq-fe.h>

#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/span.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/result_wrapper.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/storages/postgres/dsn.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

class PGConnectionWrapper {
 public:
  using Deadline = engine::Deadline;
  using Duration = Deadline::TimePoint::clock::duration;
  using ResultHandle = detail::ResultWrapper::ResultHandle;

  PGConnectionWrapper(engine::TaskProcessor& tp,
                      concurrent::BackgroundTaskStorageCore& bts, uint32_t id,
                      engine::SemaphoreLock&& pool_size_lock);
  ~PGConnectionWrapper();

  PGConnectionWrapper(const PGConnectionWrapper&) = delete;
  PGConnectionWrapper& operator=(const PGConnectionWrapper&) = delete;

  ConnectionState GetConnectionState() const;

  /// Wrapper for PQserverVersion
  int GetServerVersion() const;

  /// Wrapper for PQparameterStatus
  std::string_view GetParameterStatus(const char* name) const;

  /// @brief Asynchronously connect PG instance.
  ///
  /// Start asynchronous connection and wait for it's completion (suspending
  /// current couroutine)
  void AsyncConnect(const Dsn& dsn, Deadline deadline, tracing::ScopeTime&);

  /// @brief Causes a connection to enter pipeline mode.
  ///
  /// Pipeline mode allows applications to send a query without having to read
  /// the result of the previously sent query.
  ///
  /// Requires libpq >= 14.
  void EnterPipelineMode();

  /// @brief Returns true if command send queue is empty.
  ///
  /// Normally command queue is flushed after any Send* call, but in pipeline
  /// mode that might not be the case.
  bool IsSyncingPipeline() const;

  /// Check if pipeline mode is currently enabled
  bool IsPipelineActive() const;

  /// @brief Close the connection on a background task processor.
  [[nodiscard]] engine::Task Close();

  /// @brief Cancel current operation on a background task processor.
  [[nodiscard]] engine::Task Cancel();

  /// @brief Wrapper for PQsendQuery
  void SendQuery(const std::string& statement, tracing::ScopeTime&);

  /// @brief Wrapper for PQsendQueryParams
  void SendQuery(const std::string& statement, const QueryParameters& params,
                 tracing::ScopeTime&);

  /// @brief Wrapper for PQsendPrepare
  void SendPrepare(const std::string& name, const std::string& statement,
                   const QueryParameters& params, tracing::ScopeTime&);

  /// @brief Wrapper for PQsendDescribePrepared
  void SendDescribePrepared(const std::string& name, tracing::ScopeTime&);

  /// @brief Wrapper for PQsendQueryPrepared
  void SendPreparedQuery(const std::string& name, const QueryParameters& params,
                         tracing::ScopeTime&);

  /// @brief Wrapper for PQXSendPortalBind
  void SendPortalBind(const std::string& statement_name,
                      const std::string& portal_name,
                      const QueryParameters& params, tracing::ScopeTime&);

  /// @brief Wrapper for PQXSendPortalExecute
  void SendPortalExecute(const std::string& portal_name, std::uint32_t n_rows,
                         tracing::ScopeTime&);

  /// @brief Wait for query result
  /// Will return result or throw an exception
  ResultSet WaitResult(Deadline deadline, tracing::ScopeTime&);

  /// Consume input from connection
  void ConsumeInput(Deadline deadline);
  /// Consume all input discarding all result sets
  void DiscardInput(Deadline deadline);

  /// Consume input while the connection is busy.
  /// If the connection still busy, return false
  bool TryConsumeInput(Deadline deadline);

  /// @brief Fills current span with connection info
  void FillSpanTags(tracing::Span&) const;

  /// Logs a server-originated notice
  void LogNotice(const PGresult*) const;

  TimeoutDuration GetIdleDuration() const;

  void MarkAsBroken();

 private:
  PGTransactionStatusType GetTransactionStatus() const;

  void StartAsyncConnect(const Dsn& dsn);

  /// @throws ConnectionTimeoutError if was awakened by the deadline
  void WaitConnectionFinish(Deadline deadline, const Dsn& dsn);

  /// @throws ConnectionFailed if conn_ does not correspond to a socket
  void RefreshSocket(const Dsn& dsn);

  /// @return true if wait was successful, false if was awakened by the deadline
  [[nodiscard]] bool WaitSocketWriteable(Deadline deadline);

  /// @return true if wait was successful, false if was awakened by the deadline
  [[nodiscard]] bool WaitSocketReadable(Deadline deadline);

  void Flush(Deadline deadline);

  ResultSet MakeResult(ResultHandle&& handle);

  template <typename ExceptionType>
  void CheckError(const std::string& cmd, int pg_dispatch_result);

  void HandleSocketPostClose();

  template <typename ExceptionType>
  [[noreturn]] void CloseWithError(ExceptionType&& ex);

  void UpdateLastUse();

  engine::TaskProcessor& bg_task_processor_;
  concurrent::BackgroundTaskStorageCore& bg_task_storage_;

  PGconn* conn_{nullptr};
  engine::io::Socket socket_;
  logging::LogExtra log_extra_;
  engine::SemaphoreLock pool_size_lock_;
  std::chrono::steady_clock::time_point last_use_;
  bool is_broken_{false};
  bool is_syncing_pipeline_{false};
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
