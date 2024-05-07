#include <userver/ydb/response.hpp>

#include <ydb-cpp-sdk/client/proto/accessor.h>
#include <ydb-cpp-sdk/client/result/result.h>
#include <ydb-cpp-sdk/client/table/table.h>

#include <userver/ydb/builder.hpp>
#include <userver/ydb/exceptions.hpp>

#include <ydb/impl/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

////////////////////////////////////////////////////////////////////////////////

namespace impl {

ParseState::ParseState(const NYdb::TResultSet& result_set)
    : parser(result_set) {}

}  // namespace impl

////////////////////////////////////////////////////////////////////////////////

Row::Row(impl::ParseState& parse_state)
    : parse_state_(parse_state)
#ifndef NDEBUG
      ,
      consumed_columns_(parse_state_.parser.ColumnsCount(), false)
#endif
{
}

NYdb::TValueParser& Row::GetColumn(std::size_t index) {
  return parse_state_.parser.ColumnParser(index);
}

NYdb::TValueParser& Row::GetColumn(std::string_view name) {
  return parse_state_.parser.ColumnParser(impl::ToString(name));
}

void Row::ConsumedColumnsCheck(std::size_t column_index) {
  UASSERT_MSG(!consumed_columns_[column_index],
              fmt::format("It is allowed to take the value of column only "
                          "once. Index {} is already consumed",
                          column_index));
  consumed_columns_[column_index] = true;
}

////////////////////////////////////////////////////////////////////////////////

CursorIterator::CursorIterator(Cursor& cursor)
    : parse_state_(&*cursor.parse_state_) {}

Row CursorIterator::operator*() const {
  UASSERT(parse_state_ != nullptr);
  return Row{*parse_state_};
}

CursorIterator& CursorIterator::operator++() {
  UASSERT(parse_state_ != nullptr);
  if (!parse_state_->parser.TryNextRow()) {
    parse_state_ = nullptr;
  }
  return *this;
}

void CursorIterator::operator++(int) { ++*this; }

#if __cpp_lib_ranges >= 201911L
bool CursorIterator::operator==(const std::default_sentinel_t&) const noexcept {
  return parse_state_ == nullptr;
}
#else
bool CursorIterator::operator==(const CursorIterator& other) const noexcept {
  UASSERT(!parse_state_ || !other.parse_state_);
  return parse_state_ == nullptr && other.parse_state_ == nullptr;
}
#endif

////////////////////////////////////////////////////////////////////////////////

Cursor::Cursor(const NYdb::TResultSet& result_set)
    : truncated_(result_set.Truncated()),
      parse_state_(utils::MakeUniqueRef<impl::ParseState>(result_set)) {}

size_t Cursor::ColumnsCount() const {
  return parse_state_->parser.ColumnsCount();
}

size_t Cursor::RowsCount() const { return parse_state_->parser.RowsCount(); }

bool Cursor::IsTruncated() const { return truncated_; }

Row Cursor::GetFirstRow() {
  CursorIterator b = begin();
  if (b == end()) {
    throw EmptyResponseError{"Expected a non-empty cursor"};
  }
  return *b;
}

bool Cursor::empty() const { return size() == 0; }

std::size_t Cursor::size() const { return RowsCount(); }

CursorIterator Cursor::begin() {
  if (is_consumed_) {
    throw EmptyResponseError{
        "Expected that the cursor was not consumed before"};
  }
  is_consumed_ = true;
  CursorIterator b{*this};
  ++b;
  return b;
}

#if __cpp_lib_ranges >= 201911L
std::default_sentinel_t Cursor::end() { return std::default_sentinel; }
#else
CursorIterator Cursor::end() { return CursorIterator(); }
#endif

////////////////////////////////////////////////////////////////////////////////

namespace {

std::optional<NYdb::NTable::TQueryStats> GetStats(
    const NYdb::NTable::TDataQueryResult& query_result) {
  if (auto query_stats = query_result.GetStats(); query_stats) {
    return std::move(*query_stats);
  }
  return {};
}

}  // namespace

ExecuteResponse::ExecuteResponse(NYdb::NTable::TDataQueryResult&& query_result)
    : query_stats_(GetStats(query_result)),
      result_sets_(std::move(query_result).ExtractResultSets()) {}

std::size_t ExecuteResponse::GetCursorCount() const {
  return result_sets_.size();
}

Cursor ExecuteResponse::GetCursor(std::size_t index) const {
  EnsureResultSetsNotEmpty();

  if (GetCursorCount() <= index) {
    throw BaseError(fmt::format("No cursor with index {}", index));
  }

  return Cursor{result_sets_[index]};
}

Cursor ExecuteResponse::GetSingleCursor() const {
  EnsureResultSetsNotEmpty();

  if (result_sets_.size() != 1) {
    throw IgnoreResultsError(fmt::format(
        "There are {} results, but expected single", result_sets_.size()));
  }

  return Cursor{result_sets_.front()};
}

const std::optional<NYdb::NTable::TQueryStats>& ExecuteResponse::GetQueryStats()
    const noexcept {
  return query_stats_;
}

bool ExecuteResponse::IsFromServerQueryCache() const noexcept {
  if (!query_stats_) return false;
  // TProtoAccessor is half-private API, this access was "blessed" by YDB devs.
  const auto& stats_raw = NYdb::TProtoAccessor::GetProto(*query_stats_);
  if (!stats_raw.has_compilation()) return false;
  return stats_raw.compilation().from_cache();
}

void ExecuteResponse::EnsureResultSetsNotEmpty() const {
  if (result_sets_.empty()) {
    throw EmptyResponseError{"There are no result sets in ExecuteResponse"};
  }
}

///////////////////////////////////////////////////////////////////////

ReadTableResults::ReadTableResults(NYdb::NTable::TTablePartIterator iterator)
    : iterator_{std::move(iterator)} {}

std::optional<Cursor> ReadTableResults::GetNextResult() {
  auto status = impl::GetFutureValueUnchecked(iterator_.ReadNext());
  if (!status.IsSuccess()) {
    if (status.EOS()) return std::nullopt;
    throw YdbResponseError("ReadNext", std::move(status));
  }

  const auto& res = status.ExtractPart();
  return Cursor{res};
}

ScanQueryResults::ScanQueryResults(TScanQueryPartIterator iterator)
    : iterator_{std::move(iterator)} {}

std::optional<ScanQueryResults::TScanQueryPart>
ScanQueryResults::GetNextResult() {
  auto status = impl::GetFutureValueUnchecked(iterator_.ReadNext());
  if (!status.IsSuccess()) {
    if (status.EOS()) return std::nullopt;
    throw YdbResponseError("ReadNext", std::move(status));
  }

  return status;
}

std::optional<Cursor> ScanQueryResults::GetNextCursor() {
  while (auto result = GetNextResult()) {
    if (result->HasResultSet()) {
      return std::optional<Cursor>{std::in_place, result->ExtractResultSet()};
    }
  }
  return std::nullopt;
}

}  // namespace ydb

USERVER_NAMESPACE_END
