#pragma once

#include <optional>
#include <vector>

#include <fmt/format.h>

#include <userver/engine/deadline.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/scope_guard.hpp>

#include <userver/storages/mysql/impl/io/extractor.hpp>
#include <userver/storages/mysql/impl/io/traits.hpp>
#include <userver/storages/mysql/execution_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class MySQLStatementFetcher;
}

namespace infra {
class ConnectionPtr;
}

template <typename DbType>
class MappedStatementResultSet;

class StatementResultSet final {
 public:
  explicit StatementResultSet(impl::MySQLStatementFetcher&& fetcher);
  StatementResultSet(infra::ConnectionPtr&& connection,
                     impl::MySQLStatementFetcher&& fetcher);
  ~StatementResultSet();

  StatementResultSet(const StatementResultSet& other) = delete;
  StatementResultSet(StatementResultSet&& other) noexcept;

  template <typename T>
  std::vector<T> AsVector() &&;

  template <typename T>
  std::vector<T> AsVector(FieldTag) &&;

  template <typename Container>
  Container AsContainer() &&;

  template <typename Container>
  Container AsContainer(FieldTag) &&;

  template <typename T>
  T AsSingleRow() &&;

  template <typename T>
  T AsSingleField() &&;

  template <typename T>
  std::optional<T> AsOptionalSingleRow() &&;

  template <typename T>
  std::optional<T> AsOptionalSingleField() &&;

  template <typename DbType>
  MappedStatementResultSet<DbType> MapFrom() &&;

  ExecutionResult AsExecutionResult() &&;

 private:
  template <typename T>
  friend class CursorResultSet;

  template <typename DbType>
  friend class MappedStatementResultSet;

  template <typename Container, typename MapFrom, typename ExtractionTag>
  Container DoAsContainerMapped() &&;

  template <typename T, typename ExtractionTag>
  std::optional<T> DoAsOptionalSingleRow() &&;

  bool FetchResult(impl::io::ExtractorBase& extractor);

  struct Impl;
  utils::FastPimpl<Impl, 64, 8> impl_;
};

template <typename T>
std::vector<T> StatementResultSet::AsVector() && {
  return std::move(*this).AsContainer<std::vector<T>>();
}

template <typename T>
std::vector<T> StatementResultSet::AsVector(FieldTag) && {
  return std::move(*this).AsContainer<std::vector<T>>(kFieldTag);
}

template <typename Container>
Container StatementResultSet::AsContainer() && {
  static_assert(impl::io::kIsRange<Container>,
                "The type isn't actually a container");
  using Row = typename Container::value_type;

  return std::move(*this).DoAsContainerMapped<Container, Row, RowTag>();
}

template <typename Container>
Container StatementResultSet::AsContainer(FieldTag) && {
  static_assert(impl::io::kIsRange<Container>,
                "The type isn't actually a container");
  using Row = typename Container::value_type;

  return std::move(*this).DoAsContainerMapped<Container, Row, FieldTag>();
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
T StatementResultSet::AsSingleField() && {
  auto optional_data = std::move(*this).AsOptionalSingleField<T>();

  if (!optional_data.has_value()) {
    throw std::runtime_error{"Result set is empty"};
  }

  return std::move(*optional_data);
}

template <typename T>
std::optional<T> StatementResultSet::AsOptionalSingleRow() && {
  return std::move(*this).DoAsOptionalSingleRow<T, RowTag>();
}

template <typename T>
std::optional<T> StatementResultSet::AsOptionalSingleField() && {
  return std::move(*this).DoAsOptionalSingleRow<T, FieldTag>();
}

template <typename Container, typename MapFrom, typename ExtractionTag>
Container StatementResultSet::DoAsContainerMapped() && {
  static_assert(impl::io::kIsRange<Container>,
                "The type isn't actually a container");
  using Row = typename Container::value_type;
  using Extractor = impl::io::TypedExtractor<Row, MapFrom, ExtractionTag>;

  Extractor extractor{};

  tracing::ScopeTime fetch{"fetch"};
  std::move(*this).FetchResult(extractor);
  fetch.Reset();

  if constexpr (std::is_same_v<Container, typename Extractor::StorageType>) {
    return std::move(extractor).ExtractData();
  }

  Container container{};
  typename Extractor::StorageType rows{std::move(extractor).ExtractData()};
  if constexpr (impl::io::kIsReservable<Container>) {
    container.reserve(rows.size());
  }

  std::move(rows.begin(), rows.end(), impl::io::Inserter(container));
  return container;
}

template <typename T, typename ExtractionTag>
std::optional<T> StatementResultSet::DoAsOptionalSingleRow() && {
  auto rows = [this] {
    if constexpr (std::is_same_v<RowTag, ExtractionTag>) {
      return std::move(*this).AsVector<T>();
    } else {
      return std::move(*this).AsVector<T>(kFieldTag);
    }
  }();

  if (rows.empty()) {
    return std::nullopt;
  }

  if (rows.size() > 1) {
    throw std::runtime_error{fmt::format(
        "There is more than one row in result set ({} rows)", rows.size())};
  }

  return {std::move(rows.front())};
}

template <typename DbType>
class MappedStatementResultSet final {
 public:
  explicit MappedStatementResultSet(StatementResultSet&& result_set);
  ~MappedStatementResultSet();

  template <typename T>
  std::vector<T> AsVector() &&;

  template <typename T>
  std::vector<T> AsVector(FieldTag) &&;

  template <typename Container>
  Container AsContainer() &&;

  template <typename Container>
  Container AsContainer(FieldTag) &&;

  /*template <typename T>
  T AsSingleRow() &&;

  template <typename T>
  std::optional<T> AsOptionalSingleRow() &&;*/

 private:
  StatementResultSet result_set_;
};

template <typename DbType>
MappedStatementResultSet<DbType>::MappedStatementResultSet(
    StatementResultSet&& result_set)
    : result_set_{std::move(result_set)} {}

template <typename DbType>
MappedStatementResultSet<DbType>::~MappedStatementResultSet() = default;

template <typename DbType>
template <typename T>
std::vector<T> MappedStatementResultSet<DbType>::AsVector() && {
  return std::move(*this).template AsContainer<std::vector<T>>();
}

template <typename DbType>
template <typename T>
std::vector<T> MappedStatementResultSet<DbType>::AsVector(FieldTag) && {
  return std::move(*this).template AsContainer<std::vector<T>>(kFieldTag);
}

template <typename DbType>
template <typename Container>
Container MappedStatementResultSet<DbType>::AsContainer() && {
  return std::move(result_set_)
      .DoAsContainerMapped<Container, DbType, RowTag>();
}

template <typename DbType>
template <typename Container>
Container MappedStatementResultSet<DbType>::AsContainer(FieldTag) && {
  return std::move(result_set_)
      .DoAsContainerMapped<Container, DbType, FieldTag>();
}

template <typename DbType>
MappedStatementResultSet<DbType> StatementResultSet::MapFrom() && {
  return MappedStatementResultSet<DbType>{std::move(*this)};
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
