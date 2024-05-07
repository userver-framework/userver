#pragma once

#include <ydb-cpp-sdk/client/result/result.h>
#include <ydb-cpp-sdk/client/table/table.h>

#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <typeinfo>

#include <userver/utils/not_null.hpp>
#include <userver/ydb/impl/cast.hpp>
#include <userver/ydb/io/insert_row.hpp>
#include <userver/ydb/io/list.hpp>
#include <userver/ydb/io/primitives.hpp>
#include <userver/ydb/io/traits.hpp>
#include <userver/ydb/types.hpp>

namespace NYdb {
class TResultSetParser;
class TResultSet;
class TValueParser;

namespace NTable {
class TDataQueryResult;
class TTablePartIterator;
}  // namespace NTable
}  // namespace NYdb

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {

template <typename T>
struct StructRowParser;

struct ParseState final {
  explicit ParseState(const NYdb::TResultSet& result_set);

  NYdb::TResultSetParser parser;
  const std::type_info* row_type_id{nullptr};
  std::unique_ptr<std::size_t[]> cpp_to_ydb_field_mapping{};
};

}  // namespace impl

using ValueType = NYdb::EPrimitiveType;

class Cursor;

class Row final {
 public:
  /// @cond
  // For internal use only.
  explicit Row(impl::ParseState& parse_state);
  /// @endcond

  Row(const Row&) = delete;
  Row(Row&&) noexcept = default;
  Row& operator=(const Row&) = delete;
  Row& operator=(Row&&) = delete;

  /// @brief Parses the whole row to `T`, which must be a struct type.
  /// ydb::kStructMemberNames must be specialized for `T`.
  ///
  /// @throws ydb::ColumnParseError on parsing error
  /// @throws ydb::ParseError on extra fields on C++ side
  /// @throws ydb::ParseError on extra fields on YDB side
  template <typename T>
  T As() &&;

  /// @brief Parses the specified column to `T`.
  /// `Get` can only be called once for each column.
  /// @throws ydb::BaseError on parsing error
  template <typename T>
  T Get(std::string_view column_name);

  /// @brief Parses the specified column to `T`.
  /// `Get` can only be called once for each column.
  /// @throws ydb::BaseError on parsing error
  template <typename T>
  T Get(std::size_t column_index);

 private:
  NYdb::TValueParser& GetColumn(std::size_t index);
  NYdb::TValueParser& GetColumn(std::string_view name);

  void ConsumedColumnsCheck(std::size_t column_index);

  impl::ParseState& parse_state_;
  std::vector<bool> consumed_columns_;
};

class CursorIterator final {
 public:
  using difference_type = std::ptrdiff_t;
  using value_type = Row;
  using reference = Row;
  using iterator_category = std::input_iterator_tag;

  CursorIterator() = default;

  CursorIterator(const CursorIterator&) = delete;
  CursorIterator(CursorIterator&&) noexcept = default;
  CursorIterator& operator=(const CursorIterator&) = delete;
  CursorIterator& operator=(CursorIterator&&) = default;

  Row operator*() const;

  CursorIterator& operator++();

  void operator++(int);

#if __cpp_lib_ranges >= 201911L
  bool operator==(const std::default_sentinel_t& other) const noexcept;
#else
  bool operator==(const CursorIterator& other) const noexcept;
#endif

 private:
  friend class Cursor;

  explicit CursorIterator(Cursor& cursor);

  impl::ParseState* parse_state_{nullptr};
};

class Cursor final {
 public:
  /// @cond
  explicit Cursor(const NYdb::TResultSet& result_set);
  /// @endcond

  Cursor(const Cursor&) = delete;
  Cursor(Cursor&&) noexcept = default;
  Cursor& operator=(const Cursor&) = delete;
  Cursor& operator=(Cursor&&) noexcept = default;

  size_t ColumnsCount() const;
  size_t RowsCount() const;
  /// @throws EmptyResponseError if GetFirstRow() or begin() called before or
  /// cursor is empty
  Row GetFirstRow();

  /// Returns true if response has been truncated to the database limit
  /// (currently 1000 rows)
  bool IsTruncated() const;

  bool empty() const;
  std::size_t size() const;

  CursorIterator begin();

#if __cpp_lib_ranges >= 201911L
  std::default_sentinel_t end();
#else
  CursorIterator end();
#endif

 private:
  friend class Row;
  friend class CursorIterator;

  bool truncated_;
  bool is_consumed_{false};
  // Row should not be invalidated after moving Cursor, hence the indirection.
  utils::UniqueRef<impl::ParseState> parse_state_;
};

class ExecuteResponse final {
 public:
  /// @cond
  explicit ExecuteResponse(NYdb::NTable::TDataQueryResult&& query_result);
  /// @endcond

  ExecuteResponse(const ExecuteResponse&) = delete;
  ExecuteResponse(ExecuteResponse&&) noexcept = default;
  ExecuteResponse& operator=(const ExecuteResponse&) = delete;
  ExecuteResponse& operator=(ExecuteResponse&&) = delete;

  std::size_t GetCursorCount() const;
  Cursor GetCursor(std::size_t index) const;
  Cursor GetSingleCursor() const;

  /// Query stats are only available if initially requested
  const std::optional<NYdb::NTable::TQueryStats>&  //
  GetQueryStats() const noexcept;

  /// Returns true if Execute used the server query cache
  bool IsFromServerQueryCache() const noexcept;

 private:
  void EnsureResultSetsNotEmpty() const;

  std::optional<NYdb::NTable::TQueryStats> query_stats_;
  std::vector<NYdb::TResultSet> result_sets_;
};

class ReadTableResults final {
 public:
  /// @cond
  explicit ReadTableResults(NYdb::NTable::TTablePartIterator iterator);
  /// @endcond

  std::optional<Cursor> GetNextResult();

  ReadTableResults(const ReadTableResults&) = delete;
  ReadTableResults(ReadTableResults&&) noexcept = default;
  ReadTableResults& operator=(const ReadTableResults&) = delete;
  ReadTableResults& operator=(ReadTableResults&&) = delete;

 private:
  NYdb::NTable::TTablePartIterator iterator_;
};

class ScanQueryResults final {
  using TScanQueryPartIterator = NYdb::NTable::TScanQueryPartIterator;

 public:
  using TScanQueryPart = NYdb::NTable::TScanQueryPart;

  /// @cond
  explicit ScanQueryResults(TScanQueryPartIterator iterator);
  /// @endcond

  std::optional<TScanQueryPart> GetNextResult();

  std::optional<Cursor> GetNextCursor();

  ScanQueryResults(const ScanQueryResults&) = delete;
  ScanQueryResults(ScanQueryResults&&) noexcept = default;
  ScanQueryResults& operator=(const ScanQueryResults&) = delete;
  ScanQueryResults& operator=(ScanQueryResults&&) = delete;

 private:
  TScanQueryPartIterator iterator_;
};

template <typename T>
T Row::As() && {
  if (&typeid(T) != parse_state_.row_type_id) {
    parse_state_.cpp_to_ydb_field_mapping =
        impl::StructRowParser<T>::MakeCppToYdbFieldMapping(parse_state_.parser);
    parse_state_.row_type_id = &typeid(T);
  }
  return impl::StructRowParser<T>::ParseRow(
      parse_state_.parser, parse_state_.cpp_to_ydb_field_mapping);
}

template <typename T>
T Row::Get(std::string_view column_name) {
#ifndef NDEBUG
  ConsumedColumnsCheck(
      parse_state_.parser.ColumnIndex(impl::ToString(column_name)));
#endif
  auto& column = GetColumn(column_name);
  return Parse<T>(column, ParseContext{/*column_name=*/column_name});
}

template <typename T>
T Row::Get(std::size_t column_index) {
#ifndef NDEBUG
  ConsumedColumnsCheck(column_index);
#endif
  auto& column = GetColumn(column_index);
  const auto column_name = std::to_string(column_index);
  return Parse<T>(column, ParseContext{/*column_name=*/column_name});
}

}  // namespace ydb

USERVER_NAMESPACE_END
