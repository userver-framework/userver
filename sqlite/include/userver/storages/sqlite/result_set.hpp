#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <sqlite3.h>
#include <boost/pfr.hpp>

#include <userver/storages/sqlite/execution_result.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include "userver/storages/sqlite/exceptions.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class ResultSet {
 public:
  using size_type = std::size_t;

  explicit ResultSet(sqlite3_stmt* stmt, int exec_status);

  ResultSet(const ResultSet& other) = delete;
  ResultSet(ResultSet&& other) noexcept;

  ~ResultSet();

  // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// T is expected to be an aggregate of supported types.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  ///
  // clang-format on
  template <typename T>
  std::vector<T> AsVector() &&;

  // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// Result set is expected to have a single column, `T` is expected to be one
  /// of supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  ///
  // clang-format on
  template <typename T>
  std::vector<T> AsVector(FieldTag) &&;

  // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have a single row, `T` is expected to be an
  /// aggregate of supported types.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  /// throws if result set is empty or contains more than one row.
  ///
  // clang-format on
  template <typename T>
  T AsSingleRow() &&;

  // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have a single row and a single column,
  /// `T` is expected to be one of supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  /// throws if result set is empty of contains more than one row.
  ///
  // clang-format on
  template <typename T>
  T AsSingleField() &&;

  // clang-format off
  /// @brief Parse statement result as std::optional<T>.
  /// Result set is expected to have not more than one row,
  /// `T` is expected to be an aggregate of supported types.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  /// throws if result set contains more than one row.
  ///
  // clang-format on
  template <typename T>
  std::optional<T> AsOptionalSingleRow() &&;

  // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have not more than one row,
  /// `T` is expected to be one of supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  /// throws if result set contains more than one row.
  ///
  // clang-format on
  template <typename T>
  std::optional<T> AsOptionalSingleField() &&;

  /// @brief Get statement execution metadata.
  ExecutionResult AsExecutionResult() &&;

 private:
  struct Deleter {
    void operator()(sqlite3_stmt* stmt);
  };

  std::unique_ptr<sqlite3_stmt, Deleter> stmt_;
  int exec_status_;

  template <typename FieldType>
  static FieldType GetColumn(sqlite3_stmt* stmt, int column);

  template <typename T>
  static T convertRow(sqlite3_stmt* stmt);

  template <typename Tuple, std::size_t... I>
  static Tuple ConvertToTupleImpl(sqlite3_stmt* stmt,
                                  std::index_sequence<I...>);

  template <typename Tuple>
  static Tuple ConvertToTuple(sqlite3_stmt* stmt);

  template <typename T>
  static T ConvertToAggregate(sqlite3_stmt* stmt);
};

template <typename T>
std::vector<T> ResultSet::AsVector() && {
  static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
                "T must be an aggregate type or tuple-like type");
  std::vector<T> result;
  while (sqlite3_step(stmt_.get()) == SQLITE_ROW) {
    result.emplace_back(convertRow<T>(stmt_.get()));
  }
  return result;
}

template <typename T>
std::vector<T> ResultSet::AsVector(FieldTag) && {
  static_assert(std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                    std::is_same_v<T, std::string> ||
                    std::is_same_v<T, std::vector<uint8_t>>,
                "Unsupported type for AsVector(FieldTag)");

  const int column_count = sqlite3_column_count(stmt_.get());
  if (column_count != 1) {
    throw SQLiteException(
        "Result set must have exactly one column for AsVector(FieldTag)");
  }

  std::vector<T> result;
  while (exec_status_ == SQLITE_ROW) {
    result.emplace_back(GetColumn<T>(stmt_.get(), 0));
    exec_status_ = sqlite3_step(stmt_.get());
  }

  return result;
}

template <typename T>
T ResultSet::AsSingleRow() && {
  static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
                "T must be an aggregate type or tuple-like type");

  if (exec_status_ == SQLITE_DONE) {
    throw SQLiteException("Result set is empty");
  }

  T result = convertRow<T>(stmt_.get());

  exec_status_ = sqlite3_step(stmt_.get());
  if (exec_status_ == SQLITE_ROW) {
    throw SQLiteException("Result set contains more than one row");
  }

  return result;
}

template <typename T>
T ResultSet::AsSingleField() && {
  static_assert(std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                    std::is_same_v<T, std::string> ||
                    std::is_same_v<T, std::vector<uint8_t>>,
                "T must be one of the supported types: int64_t, double, "
                "std::string, std::vector<uint8_t>");

  if (exec_status_ == SQLITE_DONE) {
    throw SQLiteException("Result set is empty");
  }

  int column_count = sqlite3_column_count(stmt_.get());
  if (column_count != 1) {
    throw SQLiteException("Result set must contain exactly one column");
  }

  T result = GetColumn<T>(stmt_.get(), 0);

  exec_status_ = sqlite3_step(stmt_.get());
  if (exec_status_ == SQLITE_ROW) {
    throw SQLiteException("Result set contains more than one row");
  }

  return result;
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow() && {
  static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
                "T must be an aggregate type or tuple-like type");

  if (exec_status_ == SQLITE_DONE) {
    return std::nullopt;
  }

  T result = convertRow<T>(stmt_.get());

  exec_status_ = sqlite3_step(stmt_.get());
  if (exec_status_ == SQLITE_ROW) {
    throw SQLiteException("Result set contains more than one row");
  }

  return result;
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleField() && {
  static_assert(std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                    std::is_same_v<T, std::string> ||
                    std::is_same_v<T, std::vector<uint8_t>>,
                "T must be one of the supported types: int64_t, double, "
                "std::string, std::vector<uint8_t>");

  if (exec_status_ == SQLITE_DONE) {
    return std::nullopt;
  }

  int column_count = sqlite3_column_count(stmt_.get());
  if (column_count != 1) {
    throw SQLiteException("Result set must contain exactly one column");
  }

  T result = GetColumn<T>(stmt_.get(), 0);

  exec_status_ = sqlite3_step(stmt_.get());
  if (exec_status_ == SQLITE_ROW) {
    throw SQLiteException("Result set contains more than one row");
  }

  return result;
}

template <typename Tuple, std::size_t... I>
Tuple ResultSet::ConvertToTupleImpl(sqlite3_stmt* stmt,
                                    std::index_sequence<I...>) {
  return Tuple{GetColumn<std::tuple_element_t<I, Tuple>>(stmt, I)...};
}

template <typename Tuple>
Tuple ResultSet::ConvertToTuple(sqlite3_stmt* stmt) {
  constexpr std::size_t N = std::tuple_size_v<Tuple>;
  return ConvertToTupleImpl<Tuple>(stmt, std::make_index_sequence<N>{});
}

template <typename T>
T ResultSet::ConvertToAggregate(sqlite3_stmt* stmt) {
  T instance{};
  int column = 0;
  boost::pfr::for_each_field(instance, [&column, &stmt](auto& field) {
    using FieldType = std::decay_t<decltype(field)>;
    field = GetColumn<FieldType>(stmt, column++);
  });
  return instance;
}

template <typename T>
T ResultSet::convertRow(sqlite3_stmt* stmt) {
  if constexpr (std::is_aggregate_v<T>) {
    return ConvertToAggregate<T>(stmt);
  } else {
    return ConvertToTuple<T>(stmt);
  }
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
