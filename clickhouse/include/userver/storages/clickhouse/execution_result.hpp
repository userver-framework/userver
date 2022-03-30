#pragma once

#include <memory>
#include <type_traits>

#include <boost/pfr/core.hpp>

#include <userver/storages/clickhouse/impl/block_wrapper_fwd.hpp>
#include <userver/storages/clickhouse/io/validate.hpp>

#include <userver/storages/clickhouse/io/columns/column_wrapper.hpp>
#include <userver/storages/clickhouse/io/columns/common_columns.hpp>
#include <userver/storages/clickhouse/io/columns/nullable_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

class ExecutionResult final {
 public:
  explicit ExecutionResult(std::unique_ptr<impl::BlockWrapper>);
  ExecutionResult(ExecutionResult&&) noexcept;
  ~ExecutionResult();

  size_t GetColumnsCount() const;

  size_t GetRowsCount() const;

  template <typename T>
  T As() &&;

 private:
  template <typename MappedType>
  class Mapper final {
   public:
    explicit Mapper(impl::BlockWrapper& block) : block_{block} {}

    template <typename Field, size_t Index>
    void operator()(Field& field, std::integral_constant<size_t, Index> i) {
      using ColumnType = std::tuple_element_t<Index, MappedType>;
      static_assert(std::is_same_v<Field, typename ColumnType::container_type>);

      auto column = ColumnType{io::columns::GetWrappedColumn(block_, i)};
      field.reserve(column.Size());
      for (auto& it : column) field.push_back(std::move(it));
    }

   private:
    impl::BlockWrapper& block_;
  };

  std::unique_ptr<impl::BlockWrapper> block_;
};

template <typename T>
T ExecutionResult::As() && {
  T result{};
  io::Validate(result);

  using MappedType = typename io::CppToClickhouse<T>::mapped_type;
  auto mapper = ExecutionResult::Mapper<MappedType>{*block_};

  boost::pfr::for_each_field(result, mapper);

  return result;
}

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
