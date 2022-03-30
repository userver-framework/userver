#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <userver/storages/clickhouse/impl/block_wrapper_fwd.hpp>
#include <userver/storages/clickhouse/io/validate.hpp>

#include <userver/storages/clickhouse/io/columns/column_wrapper.hpp>
#include <userver/storages/clickhouse/io/columns/common_columns.hpp>
#include <userver/storages/clickhouse/io/columns/nullable_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

class InsertionRequest final {
 public:
  InsertionRequest(const std::string& table_name,
                   const std::vector<std::string_view>& column_names);
  InsertionRequest(InsertionRequest&&) noexcept;
  ~InsertionRequest();

  template <typename T>
  static InsertionRequest Create(
      const std::string& table_name,
      const std::vector<std::string_view>& column_names, const T& data);

  const std::string& GetTableName() const;

  const impl::BlockWrapper& GetBlock() const;

 private:
  template <typename MappedType>
  class Mapper final {
   public:
    Mapper(impl::BlockWrapper& block,
           const std::vector<std::string_view>& column_names)
        : block_{block}, column_names_{column_names} {}

    template <typename Field, size_t Index>
    void operator()(const Field& field,
                    std::integral_constant<size_t, Index> i) {
      using ColumnType = std::tuple_element_t<Index, MappedType>;
      static_assert(std::is_same_v<Field, typename ColumnType::container_type>);

      io::columns::AppendWrappedColumn(block_, ColumnType::Serialize(field),
                                       column_names_[i], i);
    }

   private:
    impl::BlockWrapper& block_;
    const std::vector<std::string_view>& column_names_;
  };

  const std::string& table_name_;
  const std::vector<std::string_view>& column_names_;

  std::unique_ptr<impl::BlockWrapper> block_;
};

template <typename T>
InsertionRequest InsertionRequest::Create(
    const std::string& table_name,
    const std::vector<std::string_view>& column_names, const T& data) {
  io::Validate(data);
  io::ValidateRowsCount(data);

  InsertionRequest request{table_name, column_names};
  using MappedType = typename io::CppToClickhouse<T>::mapped_type;
  auto mapper = InsertionRequest::Mapper<MappedType>{*request.block_,
                                                     request.column_names_};

  boost::pfr::for_each_field(data, mapper);

  return request;
}

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
