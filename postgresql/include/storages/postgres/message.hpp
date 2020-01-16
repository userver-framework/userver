#pragma once

/// @file storages/postgres/message.hpp
/// @brief Database messages

#include <boost/optional.hpp>
#include <string>

#include <storages/postgres/postgres_fwd.hpp>
#include <storages/postgres/sql_state.hpp>

#include <logging/log_extra.hpp>

namespace storages {
namespace postgres {

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

 public:
  explicit Message(detail::ResultWrapperPtr);

  std::string GetMessage() const;
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

  // TODO Replace with string_view
  static Severity SeverityFromString(const std::string&);

 private:
  detail::ResultWrapperPtr res_;
};

}  // namespace postgres
}  // namespace storages
