#pragma once

/// @file userver/storages/mysql/command_result_set.hpp

#include <string>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class QueryResult;
}

/// @brief A wrapper for command result set.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::mysql::Cluster.
///
/// This class doesn't come with much functionality, reach for prepared
/// statements interface of the storages::mysql::Cluster if you want some.
class CommandResultSet final {
 public:
  explicit CommandResultSet(impl::QueryResult&& mysql_result);
  ~CommandResultSet();

  CommandResultSet(const CommandResultSet& other) = delete;
  CommandResultSet(CommandResultSet&& other) noexcept;

  /// @brief Returns amount of rows in the result set.
  std::size_t RowsCount() const;

  /// @brief Returns amount of columns in the result set.
  std::size_t FieldsCount() const;

  /// @brief Return true if there are any rows in the result set,
  /// false otherwise.
  bool Empty() const;

  /// @brief Returns a value at row `row` at column `column`.
  const std::string& At(std::size_t row, std::size_t column) const;

  /// @brief Returns a value at row `row` at column `column`.
  std::string& At(std::size_t row, std::size_t column);

 private:
  class Impl;
  utils::FastPimpl<Impl, 24, 8> impl_;
};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
