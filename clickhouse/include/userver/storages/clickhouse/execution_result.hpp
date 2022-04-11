#pragma once

#include <memory>
#include <type_traits>

#include <boost/pfr/core.hpp>

#include <userver/storages/clickhouse/impl/block_wrapper_fwd.hpp>
#include <userver/storages/clickhouse/io/impl/validate.hpp>

#include <userver/storages/clickhouse/io/result_mapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

class ExecutionResult final {
 public:
  explicit ExecutionResult(impl::BlockWrapperPtr);
  ExecutionResult(ExecutionResult&&) noexcept;
  ~ExecutionResult();

  size_t GetColumnsCount() const;

  size_t GetRowsCount() const;

  template <typename T>
  T As() &&;

  template <typename T>
  auto AsRows() &&;

  template <typename Container>
  Container AsContainer() &&;

 private:
  impl::BlockWrapperPtr block_;
};

template <typename T>
T ExecutionResult::As() && {
  UASSERT(block_);
  T result{};
  io::impl::ValidateColumnsMapping(result);
  io::impl::ValidateColumnsCount<T>(GetColumnsCount());

  using MappedType = typename io::CppToClickhouse<T>::mapped_type;
  io::ColumnsMapper<MappedType> mapper{*block_};

  boost::pfr::for_each_field(result, mapper);

  return result;
}

template <typename T>
auto ExecutionResult::AsRows() && {
  UASSERT(block_);
  io::impl::ValidateRowsMapping<T>();
  io::impl::ValidateColumnsCount<T>(GetColumnsCount());

  return io::RowsMapper<T>{std::move(block_)};
}

template <typename Container>
Container ExecutionResult::AsContainer() && {
  UASSERT(block_);
  using Row = typename Container::value_type;

  Container result;
  if constexpr (io::traits::kIsReservable<Container>) {
    result.reserve(GetRowsCount());
  }

  auto rows = std::move(*this).AsRows<Row>();
  std::move(rows.begin(), rows.end(), io::traits::Inserter(result));
  return result;
}

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
