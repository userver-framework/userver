#pragma once

/// @file userver/storages/clickhouse/execution_result.hpp
/// @brief Result accessor.

#include <memory>
#include <type_traits>

#include <boost/pfr/core.hpp>

#include <userver/storages/clickhouse/impl/block_wrapper_fwd.hpp>
#include <userver/storages/clickhouse/io/impl/validate.hpp>

#include <userver/storages/clickhouse/io/result_mapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

// clang-format off

/// Thin wrapper over underlying block of data, returned by
/// storages::clickhouse::Cluster Execute methods
///
/// ## Usage example:
///
/// @snippet storages/tests/execute_chtest.cpp  Sample CppToClickhouse specialization
///
/// @snippet storages/tests/execute_chtest.cpp  Sample ExecutionResult usage

// clang-format on
class ExecutionResult final {
 public:
  explicit ExecutionResult(impl::BlockWrapperPtr);
  ExecutionResult(ExecutionResult&&) noexcept;
  ~ExecutionResult();

  /// Returns number of columns in underlying block.
  size_t GetColumnsCount() const;

  /// Returns number of rows in underlying block columns.
  size_t GetRowsCount() const;

  /// Converts underlying block to strongly-typed struct of vectors.
  /// See @ref clickhouse_io for better understanding of `T`'s requirements.
  template <typename T>
  T As() &&;

  /// Converts underlying block to iterable of strongly-typed struct.
  /// See @ref clickhouse_io for better understanding of `T`'s requirements.
  template <typename T>
  auto AsRows() &&;

  /// Converts underlying block to strongly-typed container.
  /// See @ref clickhouse_io for better understanding
  /// of `Container::value_type`'s requirements.
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
