#pragma once

#include <memory>

#include <sqlite3.h>
#include <boost/pfr.hpp>

#include <userver/storages/sqlite/row_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

/// @brief Wrapper for executed sqlite3_stmt
class ResultWrapper {
 public:
  ResultWrapper(std::shared_ptr<sqlite3_stmt> stmt, int exec_status);
  ~ResultWrapper();

  int RowsAffected() const noexcept;

  int LastInsertRowId() const noexcept;

  bool HasNext() const noexcept;

  bool IsDone() const noexcept;

  void Next() noexcept;

  int ColumnCount() const noexcept;

  template <typename T>
  T FetchNext();

  template <typename RowType>
  RowType FetchNext(RowTag);

  template <typename FieldType>
  FieldType FetchNext(FieldTag);

 private:
  std::shared_ptr<sqlite3_stmt> stmt_;
  int exec_status_;  // TODO: We can get it from sqlite3_errcode, but it maybe
                     // unsafe in a multi-thread environment or with invested
                     // queries

  template <typename FieldType>
  static FieldType GetColumn(sqlite3_stmt* stmt, int column);

  template <typename T>
  static T ConvertRow(sqlite3_stmt* stmt);

  template <typename Tuple, std::size_t... I>
  static Tuple ConvertToTupleImpl(sqlite3_stmt* stmt,
                                  std::index_sequence<I...>);

  template <typename Tuple>
  static Tuple ConvertToTuple(sqlite3_stmt* stmt);

  template <typename T>
  static T ConvertToAggregate(sqlite3_stmt* stmt);
};

template <typename T>
T ResultWrapper::FetchNext() {
  return FetchNext<T>(kRowTag);
}

template <typename RowType>
RowType ResultWrapper::FetchNext(RowTag) {
  auto row = ConvertRow<RowType>(stmt_.get());
  Next();
  return row;
}

template <typename FieldType>
FieldType ResultWrapper::FetchNext(FieldTag) {
  auto column = GetColumn<FieldType>(stmt_.get(), 0);
  Next();
  return column;
}

template <typename Tuple, std::size_t... I>
Tuple ResultWrapper::ConvertToTupleImpl(sqlite3_stmt* stmt,
                                        std::index_sequence<I...>) {
  return Tuple{GetColumn<std::tuple_element_t<I, Tuple>>(stmt, I)...};
}

template <typename Tuple>
Tuple ResultWrapper::ConvertToTuple(sqlite3_stmt* stmt) {
  constexpr std::size_t N = std::tuple_size_v<Tuple>;
  return ConvertToTupleImpl<Tuple>(stmt, std::make_index_sequence<N>{});
}

template <typename T>
T ResultWrapper::ConvertToAggregate(sqlite3_stmt* stmt) {
  T instance{};
  int column = 0;
  boost::pfr::for_each_field(instance, [&column, &stmt](auto&& field) {
    using FieldType = std::decay_t<decltype(field)>;
    field = GetColumn<FieldType>(stmt, column++);
  });
  return instance;
}

template <typename T>
T ResultWrapper::ConvertRow(sqlite3_stmt* stmt) {
  if constexpr (std::is_aggregate_v<T>) {
    return ConvertToAggregate<T>(stmt);
  } else {
    return ConvertToTuple<T>(stmt);
  }
}

template <>
inline int32_t ResultWrapper::GetColumn<int32_t>(sqlite3_stmt* stmt,
                                                 int column) {
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  return sqlite3_column_int(stmt, column);
}

template <>
inline uint32_t ResultWrapper::GetColumn<uint32_t>(sqlite3_stmt* stmt,
                                                   int column) {
  // TODO: Check for null, what to return if the value is null, not 0
  return sqlite3_column_int64(stmt, column);
}

template <>
inline int64_t ResultWrapper::GetColumn<int64_t>(sqlite3_stmt* stmt,
                                                 int column) {
  return sqlite3_column_int64(stmt, column);
}

template <>
inline double ResultWrapper::GetColumn<double>(sqlite3_stmt* stmt, int column) {
  return sqlite3_column_double(stmt, column);
}

template <>
inline const char* ResultWrapper::GetColumn<const char*>(sqlite3_stmt* stmt,
                                                         int column) {
  // Return a pointer to the text value (NULL terminated string) of the column
  // specified by its index starting at 0
  auto text_ptr =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt, column));
  return text_ptr ? text_ptr : "";
}

template <>
inline std::string ResultWrapper::GetColumn<std::string>(sqlite3_stmt* stmt,
                                                         int column) {
  // Note: using sqlite3_column_blob and not sqlite3_column_text
  // - no need for sqlite3_column_text to add a \0 on the end, as we're getting
  // the bytes length directly
  //   however, we need to call sqlite3_column_bytes() to ensure correct format.
  //   It's a noop on a BLOB or a TEXT value with the correct encoding (UTF-8).
  //   Otherwise it'll do a conversion to TEXT (UTF-8).
  // (void)sqlite3_column_bytes(stmt, column);
  auto data = static_cast<const char*>(sqlite3_column_blob(stmt, column));
  // SQLite docs: "The safest policy is to invoke… sqlite3_column_blob()
  // followed by sqlite3_column_bytes()"
  // Note: std::string is ok to pass nullptr as first arg, if length is 0
  return std::string(data, sqlite3_column_bytes(stmt, column));
}

template <>
inline const void* ResultWrapper::GetColumn<const void*>(sqlite3_stmt* stmt,
                                                         int column) {
  return sqlite3_column_blob(stmt, column);
}

template <>
inline std::vector<uint8_t> ResultWrapper::GetColumn<std::vector<uint8_t>>(
    sqlite3_stmt* stmt, int column) {
  const void* blob = sqlite3_column_blob(stmt, column);
  int size = sqlite3_column_bytes(stmt, column);
  return blob ? std::vector<uint8_t>(static_cast<const uint8_t*>(blob),
                                     static_cast<const uint8_t*>(blob) + size)
              : std::vector<uint8_t>{};
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
