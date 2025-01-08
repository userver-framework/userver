#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
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

  explicit ResultSet(sqlite3_stmt* stmt) : stmt_(stmt) {
    if (!stmt_) throw SQLiteException("Statement cannot be null");
  }

  ~ResultSet() = default;

  struct Deleter {
    void operator()(sqlite3_stmt* stmt) { sqlite3_finalize(stmt); }
  };

  // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// T is expected to be an aggregate of supported types.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  ///
  // clang-format on
  template <typename T>
  std::vector<T> AsVector() && {
    std::vector<T> result;
    while (sqlite3_step(stmt_.get()) == SQLITE_ROW) {
      result.push_back(convertRow<T>(stmt_.get()));
    }
    return result;
  }

  // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// Result set is expected to have a single column, `T` is expected to be one
  /// of supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  ///
  // clang-format on
  template <typename T>
  std::vector<T> AsVector(FieldTag) && {
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
    while (sqlite3_step(stmt_.get()) == SQLITE_ROW) {
      result.push_back(GetColumn<T>(stmt_.get(), 0));
    }

    return result;
  }

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
  T AsSingleRow() && {
    static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
                  "T must be an aggregate type or tuple-like type");

    int step_result = sqlite3_step(stmt_.get());
    if (step_result == SQLITE_DONE) {
      throw SQLiteException("Result set is empty");
    } else if (step_result != SQLITE_ROW) {
      throw SQLiteException("Failed to fetch row");
    }

    T result = convertRow<T>(stmt_.get());

    step_result = sqlite3_step(stmt_.get());
    if (step_result == SQLITE_ROW) {
      throw SQLiteException("Result set contains more than one row");
    }

    return result;
  }

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
  T AsSingleField() && {
    static_assert(std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                      std::is_same_v<T, std::string> ||
                      std::is_same_v<T, std::vector<uint8_t>>,
                  "T must be one of the supported types: int64_t, double, "
                  "std::string, std::vector<uint8_t>");

    int step_result = sqlite3_step(stmt_.get());
    if (step_result == SQLITE_DONE) {
      throw SQLiteException("Result set is empty");
    } else if (step_result != SQLITE_ROW) {
      throw SQLiteException("Failed to fetch row");
    }

    int column_count = sqlite3_column_count(stmt_.get());
    if (column_count != 1) {
      throw SQLiteException("Result set must contain exactly one column");
    }

    T result = GetColumn<T>(stmt_.get(), 0);

    step_result = sqlite3_step(stmt_.get());
    if (step_result == SQLITE_ROW) {
      throw SQLiteException("Result set contains more than one row");
    }

    return result;
  }

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
  std::optional<T> AsOptionalSingleRow() && {
    static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
                  "T must be an aggregate type or tuple-like type");

    int step_result = sqlite3_step(stmt_.get());
    if (step_result == SQLITE_DONE || step_result != SQLITE_ROW) {
      return std::nullopt;
    }

    T result = convertRow<T>(stmt_.get());

    step_result = sqlite3_step(stmt_.get());
    if (step_result == SQLITE_ROW) {
      throw SQLiteException("Result set contains more than one row");
    }

    return result;
  }

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
  std::optional<T> AsOptionalSingleField() && {
    static_assert(std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                      std::is_same_v<T, std::string> ||
                      std::is_same_v<T, std::vector<uint8_t>>,
                  "T must be one of the supported types: int64_t, double, "
                  "std::string, std::vector<uint8_t>");

    int step_result = sqlite3_step(stmt_.get());
    if (step_result == SQLITE_DONE || step_result != SQLITE_ROW) {
      return std::nullopt;
    }

    int column_count = sqlite3_column_count(stmt_.get());
    if (column_count != 1) {
      throw SQLiteException("Result set must contain exactly one column");
    }

    T result = GetColumn<T>(stmt_.get(), 0);

    step_result = sqlite3_step(stmt_.get());
    if (step_result == SQLITE_ROW) {
      throw SQLiteException("Result set contains more than one row");
    }

    return result;
  }

  /// @brief Get statement execution metadata.
  ExecutionResult AsExecutionResult() && {
    const auto rows_affected = sqlite3_changes(sqlite3_db_handle(stmt_.get()));
    const auto last_insert_id =
        sqlite3_last_insert_rowid(sqlite3_db_handle(stmt_.get()));

    ExecutionResult result{};
    result.rows_affected = rows_affected;
    result.last_insert_id = last_insert_id;
    return result;
  }

 private:
  std::unique_ptr<sqlite3_stmt, Deleter> stmt_;

  template <typename FieldType>
  static FieldType GetColumn(sqlite3_stmt* stmt, int column);

  template <typename T>
  static T convertRow(sqlite3_stmt* stmt);

  template <typename Tuple, std::size_t... I>
  static Tuple ConvertToTupleImpl(sqlite3_stmt* stmt,
                                  std::index_sequence<I...>) {
    return Tuple{GetColumn<std::tuple_element_t<I, Tuple>>(stmt, I)...};
  }

  template <typename Tuple>
  static Tuple ConvertToTuple(sqlite3_stmt* stmt) {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    return ConvertToTupleImpl<Tuple>(stmt, std::make_index_sequence<N>{});
  }

  template <typename T, typename Func>
  static void ForEachField(T& obj, Func&& func) {
    boost::pfr::for_each_field(obj, std::forward<Func>(func));
  }

  template <typename T>
  static T ConvertToAggregate(sqlite3_stmt* stmt) {
    T instance{};
    int column = 0;
    ForEachField(instance, [&column, &stmt](auto& field) {
      using FieldType = std::decay_t<decltype(field)>;
      field = GetColumn<FieldType>(stmt, column++);
    });
    return instance;
  }
};

template <typename T>
T ResultSet::convertRow(sqlite3_stmt* stmt) {
  if constexpr (std::is_aggregate_v<T>) {
    return ConvertToAggregate<T>(stmt);
  } else {
    return ConvertToTuple<T>(stmt);
  }
}

template <>
inline int64_t ResultSet::GetColumn<int64_t>(sqlite3_stmt* stmt, int column) {
  return sqlite3_column_int64(stmt, column);
}

template <>
inline double ResultSet::GetColumn<double>(sqlite3_stmt* stmt, int column) {
  return sqlite3_column_double(stmt, column);
}

template <>
inline std::string ResultSet::GetColumn<std::string>(sqlite3_stmt* stmt,
                                                     int column) {
  const char* text =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt, column));
  return text ? text : "";
}

template <>
inline std::vector<uint8_t> ResultSet::GetColumn<std::vector<uint8_t>>(
    sqlite3_stmt* stmt, int column) {
  const void* blob = sqlite3_column_blob(stmt, column);
  int size = sqlite3_column_bytes(stmt, column);
  return blob ? std::vector<uint8_t>(static_cast<const uint8_t*>(blob),
                                     static_cast<const uint8_t*>(blob) + size)
              : std::vector<uint8_t>{};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
