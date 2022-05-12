#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <userver/utils/assert.hpp>

#include <userver/storages/clickhouse/impl/block_wrapper_fwd.hpp>
#include <userver/storages/clickhouse/io/impl/validate.hpp>

#include <userver/storages/clickhouse/io/columns/column_wrapper.hpp>
#include <userver/storages/clickhouse/io/columns/common_columns.hpp>
#include <userver/storages/clickhouse/io/columns/nullable_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

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

  template <typename Container>
  static InsertionRequest CreateFromRows(
      const std::string& table_name,
      const std::vector<std::string_view>& column_names, const Container& data);

  const std::string& GetTableName() const;

  const impl::BlockWrapper& GetBlock() const;

 private:
  template <typename MappedType>
  class ColumnsMapper final {
   public:
    ColumnsMapper(impl::BlockWrapper& block,
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

  template <typename MappedType, typename Container>
  class RowsMapper final {
   public:
    RowsMapper(impl::BlockWrapper& block,
               const std::vector<std::string_view>& column_names,
               const Container& data)
        : block_{block}, column_names_{column_names}, data_{data} {}

    template <typename Field, size_t Index>
    void operator()(const Field&, std::integral_constant<size_t, Index> i) {
      using ColumnType = std::tuple_element_t<Index, MappedType>;
      static_assert(std::is_same_v<Field, typename ColumnType::cpp_type>);

      std::vector<Field> column_data;
      column_data.reserve(data_.size());
      for (const auto& row : data_) {
        column_data.emplace_back(boost::pfr::get<Index>(row));
      }

      io::columns::AppendWrappedColumn(
          block_, ColumnType::Serialize(column_data), column_names_[i], i);
    }

   private:
    impl::BlockWrapper& block_;
    const std::vector<std::string_view>& column_names_;
    const Container& data_;
  };

  const std::string& table_name_;
  const std::vector<std::string_view>& column_names_;

  std::unique_ptr<impl::BlockWrapper> block_;
};

template <typename T>
InsertionRequest InsertionRequest::Create(
    const std::string& table_name,
    const std::vector<std::string_view>& column_names, const T& data) {
  io::impl::ValidateColumnsMapping(data);
  io::impl::ValidateRowsCount(data);
  // TODO : static_assert this when std::span comes
  io::impl::ValidateColumnsCount<T>(column_names.size());

  InsertionRequest request{table_name, column_names};
  using MappedType = typename io::CppToClickhouse<T>::mapped_type;
  auto mapper = InsertionRequest::ColumnsMapper<MappedType>{
      *request.block_, request.column_names_};

  boost::pfr::for_each_field(data, mapper);
  return request;
}

template <typename Container>
InsertionRequest InsertionRequest::CreateFromRows(
    const std::string& table_name,
    const std::vector<std::string_view>& column_names, const Container& data) {
  using T = typename Container::value_type;
  io::impl::CommonValidateMapping<T>();
  // TODO : static_assert this when std::span comes
  io::impl::ValidateColumnsCount<T>(column_names.size());

  UINVARIANT(!data.empty(), "An attempt to insert empty chunk of data");

  InsertionRequest request{table_name, column_names};
  using MappedType = typename io::CppToClickhouse<T>::mapped_type;
  auto mapper = InsertionRequest::RowsMapper<MappedType, Container>{
      *request.block_, request.column_names_, data};

  boost::pfr::for_each_field(data.front(), mapper);
  return request;
}

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
