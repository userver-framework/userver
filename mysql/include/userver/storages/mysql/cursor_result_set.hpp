#pragma once

#include <userver/storages/mysql/statement_result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

template <typename T>
class CursorResultSet final {
 public:
  explicit CursorResultSet(StatementResultSet&& result_set);
  ~CursorResultSet();

  CursorResultSet(const CursorResultSet& other) = delete;
  CursorResultSet(CursorResultSet&& other) noexcept;

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
  bool keep_going = true;
  auto extractor = impl::io::TypedExtractor<T, T, RowTag>{};

  while (keep_going) {
    tracing::ScopeTime fetch{"fetch"};
    keep_going = result_set_.FetchResult(extractor);
    fetch.Reset();

    tracing::ScopeTime for_each{"for_each"};
    std::vector<T> data{extractor.ExtractData()};
    for (auto&& row : data) {
      row_callback(std::move(row));
    }
  }
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
