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

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class MySQLStatementFetcher;
}

namespace infra {
class ConnectionPtr;
}

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

  // TODO : swap parameters? Right now it's AsVectorMapped<To, From>,
  // when AsVectorMapped<From, To> is likely a more readable version
  template <typename T, typename MapFrom>
  std::vector<T> AsVectorMapped() &&;

  template <typename Container>
  Container AsContainer() &&;

  template <typename Container, typename MapFrom>
  Container AsContainerMapped() &&;

  template <typename T>
  T AsSingleRow() &&;

  template <typename T>
  std::optional<T> AsOptionalSingleRow() &&;

 private:
  template <typename T>
  friend class CursorResultSet;

  bool FetchResult(impl::io::ExtractorBase& extractor);

  struct Impl;
  utils::FastPimpl<Impl, 64, 8> impl_;
};

template <typename T>
std::vector<T> StatementResultSet::AsVector() && {
  return std::move(*this).AsVectorMapped<T, T>();
}

template <typename T, typename MapFrom>
std::vector<T> StatementResultSet::AsVectorMapped() && {
  return std::move(*this).AsContainerMapped<std::vector<T>, MapFrom>();
}

template <typename Container>
Container StatementResultSet::AsContainer() && {
  static_assert(impl::io::kIsRange<Container>,
                "The type isn't actually a container");
  using Row = typename Container::value_type;

  return std::move(*this).AsContainerMapped<Container, Row>();
}

template <typename Container, typename MapFrom>
Container StatementResultSet::AsContainerMapped() && {
  static_assert(impl::io::kIsRange<Container>,
                "The type isn't actually a container");
  using Row = typename Container::value_type;
  using Extractor = impl::io::TypedExtractor<Row, MapFrom>;

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

}  // namespace storages::mysql

USERVER_NAMESPACE_END
