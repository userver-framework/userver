#pragma once

/// @file userver/storages/postgres/message.hpp
/// @brief Database messages

#include <string>
#include <string_view>

#include <userver/logging/log_extra.hpp>

#include <userver/storages/postgres/postgres_fwd.hpp>
#include <userver/storages/postgres/sql_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

class Message {
 public:
  enum class Severity {
    kLog,
    kDebug,
    kInfo,
    kNotice,
    kWarning,
    kError,
    kFatal,
    kPanic
  };

  explicit Message(detail::ResultWrapperPtr);

  std::string GetMessage() const;
  std::string GetPrimary() const;
  std::string GetDetail() const;
  Severity GetSeverity() const;
  std::string GetSeverityString() const;
  SqlState GetSqlState() const;

  std::string GetSchema() const;
  std::string GetTable() const;
  std::string GetColumn() const;
  std::string GetDatatype() const;
  std::string GetConstraint() const;

  logging::LogExtra GetLogExtra() const;

  /// @brief Throw an exception according to the SQL code
  void ThrowException() const;

  static Severity SeverityFromString(std::string_view);

 private:
  detail::ResultWrapperPtr res_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
