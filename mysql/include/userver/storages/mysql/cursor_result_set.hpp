#pragma once

/// @file userver/storages/mysql/cursor_result_set.hpp

#include <userver/storages/mysql/statement_result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

/// @brief A wrapper for read-only cursor.
///
/// Although this class can be constructed from StatementResultSet, you should
/// always retrieve it from `storages::mysql::Cluster` for correct behavior.
template <typename T>
class CursorResultSet final {
 public:
  explicit CursorResultSet(StatementResultSet&& result_set);
  ~CursorResultSet();

  CursorResultSet(const CursorResultSet& other) = delete;
  CursorResultSet(CursorResultSet&& other) noexcept;

  /// @brief Fetches all the rows from cursor and for each new row executes
  /// row_callback.
  ///
  /// Usable when the result set is expected to be big enough to put too
  /// much memory pressure if fetched as a whole.
  // TODO : deadline?
  template <typename RowCallback>
  void ForEach(RowCallback&& row_callback, engine::Deadline deadline) &&;

 private:
  StatementResultSet result_set_;
};

template <typename T>
CursorResultSet<T>::CursorResultSet(StatementResultSet&& result_set)
    : result_set_{std::move(result_set)} {}

template <typename T>
CursorResultSet<T>::~CursorResultSet() = default;

template <typename T>
CursorResultSet<T>::CursorResultSet(CursorResultSet<T>&& other) noexcept =
    default;

template <typename T>
template <typename RowCallback>
void CursorResultSet<T>::ForEach(
    RowCallback&& row_callback,
    // TODO : think about separate deadline here
    [[maybe_unused]] engine::Deadline deadline) && {
  using IntermediateStorage = std::vector<T>;

  bool keep_going = true;
  auto extractor = impl::io::TypedExtractor<IntermediateStorage, T, RowTag>{};

  while (keep_going) {
    tracing::ScopeTime fetch{impl::tracing::kFetchScope};
    keep_going = result_set_.FetchResult(extractor);

    fetch.Reset(impl::tracing::kForEachScope);
    IntermediateStorage data{extractor.ExtractData()};
    for (auto&& row : data) {
      row_callback(std::move(row));
    }
  }
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
