#pragma once

#include <libpq-fe.h>
#include <chrono>

#include <engine/async.hpp>
#include <engine/io/socket.hpp>
#include <engine/task/task.hpp>
#include <logging/log_extra.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/result_wrapper.hpp>

namespace storages {
namespace postgres {
namespace detail {

class PGConnectionWrapper {
 public:
  using Duration = engine::Deadline::TimePoint::clock::duration;
  using ResultHandle = detail::ResultWrapper::ResultHandle;

 public:
  PGConnectionWrapper(engine::TaskProcessor& tp, uint32_t id);
  ~PGConnectionWrapper();

  PGConnectionWrapper(const PGConnectionWrapper&) = delete;
  PGConnectionWrapper& operator=(const PGConnectionWrapper&) = delete;

  ConnectionState GetConnectionState() const;

  // TODO Add tracing::Span
  /// @brief Asynchronously connect PG instance.
  /// Start asynchronous connection and wait for it's completion (suspending
  /// current couroutine)
  /// @param conninfo Connection string
  /// @param
  void AsyncConnect(const std::string& conninfo, Duration poll_timeout);

  /// @brief Close the connection on a background task processor.
  [[nodiscard]] engine::Task Close();
  /// @brief Cancel current operation on a background task processor.
  [[nodiscard]] engine::Task Cancel();

  // TODO Add tracing::Span
  /// @brief Wrapper for PQsendQuery
  void SendQuery(const std::string& statement);
  // TODO Add tracing::Span
  /// @brief Wrapper for PQsendQueryParams
  void SendQuery(const std::string& statement, const QueryParameters& params,
                 io::DataFormat reply_format = io::DataFormat::kTextDataFormat);

  // TODO Add tracing::Span
  /// @brief Wrapper for PQsendPrepare
  void SendPrepare(const std::string& name, const std::string& statement,
                   const QueryParameters& params);
  // TODO Add tracing::Span
  /// @brief Wrapper for PQsendDescribePrepared
  void SendDescribePrepared(const std::string& name);
  // TODO Add tracing::Span
  /// @brief Wrapper for PQsendQueryPrepared
  void SendPreparedQuery(
      const std::string& name, const QueryParameters& params,
      io::DataFormat reply_format = io::DataFormat::kTextDataFormat);
  // TODO Add tracing::Span
  /// @brief Wait for query result
  /// Will return result or throw an exception
  ResultSet WaitResult(const UserTypes&, Duration timeout);
  /// @brief Get extra log information
  /// Used for internal needs
  const logging::LogExtra& GetLogExtra() const;

 private:
  PGTransactionStatusType GetTransactionStatus() const;

  void StartAsyncConnect(const std::string& conninfo);
  void WaitConnectionFinish(Duration poll_timeout);

  void WaitSocketWriteable(Duration timeout);
  void WaitSocketReadable(Duration timeout);

  void Flush(Duration timeout);
  void ConsumeInput(Duration timeout);

  ResultSet MakeResult(const UserTypes&, ResultHandle&& handle);

  template <typename ExceptionType>
  void CheckError(const std::string& cmd, int pg_dispatch_result);
  template <typename ExceptionType>
  void CloseWithError(ExceptionType&& ex);

 private:
  engine::TaskProcessor& bg_task_processor_;

  PGconn* conn_ = nullptr;
  engine::io::Socket socket_;
  logging::LogExtra log_extra_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
