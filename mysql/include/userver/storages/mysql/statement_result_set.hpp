#pragma once

#include <optional>
#include <vector>

#include <fmt/format.h>

#include <userver/engine/deadline.hpp>
#include <userver/utils/fast_pimpl.hpp>

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
  std::vector<T> AsVector(engine::Deadline deadline) &&;

  template <typename Container>
  Container AsContainer(engine::Deadline deadline) &&;

  template <typename T>
  T AsSingleRow(engine::Deadline deadline) &&;

  template <typename T>
  std::optional<T> AsOptionalSingleRow(engine::Deadline deadline) &&;

 private:
  void FetchResult(io::ExtractorBase& extractor, engine::Deadline deadline) &&;

  struct Impl;
  utils::FastPimpl<Impl, 32, 8> impl_;
};

template <typename T>
std::vector<T> StatementResultSet::AsVector(engine::Deadline deadline) && {
  return std::move(*this).AsContainer<std::vector<T>>(deadline);
}

template <typename Container>
Container StatementResultSet::AsContainer(engine::Deadline deadline) && {
  static_assert(io::kIsRange<Container>, "The type isn't actually a container");
  using Row = typename Container::value_type;

  auto extractor = io::TypedExtractor<Row>{};
  std::move(*this).FetchResult(extractor, deadline);

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
T StatementResultSet::AsSingleRow(engine::Deadline deadline) && {
  auto optional_data = std::move(*this).AsOptionalSingleRow<T>(deadline);

  if (!optional_data.has_value()) {
    throw std::runtime_error{"Result set is empty"};
  }

  return std::move(*optional_data);
}

template <typename T>
std::optional<T> StatementResultSet::AsOptionalSingleRow(
    engine::Deadline deadline) && {
  auto rows = std::move(*this).AsContainer<std::vector<T>>(deadline);

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
