#pragma once

#include <optional>
#include <vector>

#include <fmt/format.h>

#include <userver/engine/deadline.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/scope_guard.hpp>

#include <userver/storages/mysql/io/extractor.hpp>
#include <userver/storages/mysql/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class MySQLStatementFetcher;
}

namespace infra {
class ConnectionPtr;
}

template <typename T>
class StreamedStatementResultSet;

class StatementResultSet final {
 public:
  StatementResultSet(infra::ConnectionPtr&& connection,
                     impl::MySQLStatementFetcher&& fetcher);
  ~StatementResultSet();

  StatementResultSet(const StatementResultSet& other) = delete;
  StatementResultSet(StatementResultSet&& other) noexcept;

  template <typename T>
  std::vector<T> AsVector() &&;

  template <typename Container>
  Container AsContainer() &&;

  template <typename T>
  T AsSingleRow() &&;

  template <typename T>
  std::optional<T> AsOptionalSingleRow() &&;

  template <typename T>
  StreamedStatementResultSet<T> AsStreamOf() &&;

 private:
  template <typename T>
  friend class StreamedStatementResultSet;

  bool FetchResult(io::ExtractorBase& extractor);
  void SetBatchSize(std::size_t batch_size);

  struct Impl;
  utils::FastPimpl<Impl, 64, 8> impl_;
};

template <typename T>
class StreamedStatementResultSet final {
 public:
  explicit StreamedStatementResultSet(StatementResultSet&& result_set);
  ~StreamedStatementResultSet();

  StreamedStatementResultSet(const StreamedStatementResultSet& other) = delete;
  StreamedStatementResultSet(StreamedStatementResultSet&& other) noexcept;

  template <typename RowCallback>
  void ForEach(RowCallback&& row_callback, std::size_t batch_size,
               engine::Deadline deadline) &&;

 private:
  StatementResultSet result_set_;
};

template <typename T>
std::vector<T> StatementResultSet::AsVector() && {
  return std::move(*this).AsContainer<std::vector<T>>();
}

template <typename Container>
Container StatementResultSet::AsContainer() && {
  static_assert(io::kIsRange<Container>, "The type isn't actually a container");
  using Row = typename Container::value_type;

  auto extractor = io::TypedExtractor<Row>{};
  std::move(*this).FetchResult(extractor);

  if constexpr (meta::kIsInstantiationOf<std::vector, Container>) {
    return std::move(extractor).ExtractData();
  }

  Container container{};
  auto rows = std::move(extractor).ExtractData();
  if constexpr (io::kIsReservable<Container>) {
    container.reserve(rows.size());
  }

  std::move(rows.begin(), rows.end(), io::Inserter(container));
  return container;
}

template <typename T>
T StatementResultSet::AsSingleRow() && {
  auto optional_data = std::move(*this).AsOptionalSingleRow<T>();

  if (!optional_data.has_value()) {
    throw std::runtime_error{"Result set is empty"};
  }

  return std::move(*optional_data);
}

template <typename T>
std::optional<T> StatementResultSet::AsOptionalSingleRow() && {
  auto rows = std::move(*this).AsVector<T>();

  if (rows.empty()) {
    return std::nullopt;
  }

  if (rows.size() > 1) {
    throw std::runtime_error{fmt::format(
        "There is more than one row in result set ({} rows)", rows.size())};
  }

  return {std::move(rows.front())};
}

template <typename T>
StreamedStatementResultSet<T> StatementResultSet::AsStreamOf() && {
  return StreamedStatementResultSet<T>{std::move(*this)};
}

template <typename T>
StreamedStatementResultSet<T>::StreamedStatementResultSet(
    StatementResultSet&& result_set)
    : result_set_{std::move(result_set)} {}

template <typename T>
StreamedStatementResultSet<T>::~StreamedStatementResultSet() = default;

template <typename T>
StreamedStatementResultSet<T>::StreamedStatementResultSet(
    StreamedStatementResultSet<T>&& other) noexcept = default;

template <typename T>
template <typename RowCallback>
void StreamedStatementResultSet<T>::ForEach(
    RowCallback&& row_callback, std::size_t batch_size,
    // TODO : think about separate deadline here
    engine::Deadline deadline) && {
  UASSERT(batch_size > 0);
  result_set_.SetBatchSize(batch_size);

  while (true) {
    auto extractor = io::TypedExtractor<T>{};

    if (!result_set_.FetchResult(extractor)) {
      break;
    }

    for (auto&& row : std::move(extractor).ExtractData()) {
      row_callback(std::move(row));
    }
  }
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
