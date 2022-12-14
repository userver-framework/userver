#pragma once

#include <optional>
#include <vector>

#include <fmt/format.h>

#include <userver/engine/deadline.hpp>
#include <userver/tracing/scope_time.hpp>
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

 private:
  template <typename T>
  friend class CursorResultSet;

  bool FetchResult(io::ExtractorBase& extractor);

  struct Impl;
  utils::FastPimpl<Impl, 64, 8> impl_;
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

  tracing::ScopeTime fetch{"fetch"};
  std::move(*this).FetchResult(extractor);
  fetch.Reset();

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

}  // namespace storages::mysql

USERVER_NAMESPACE_END
