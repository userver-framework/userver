#pragma once

#include <type_traits>
#include <vector>

#include <boost/pfr/core.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/meta_light.hpp>

#include <userver/storages/clickhouse/io/columns/base_column.hpp>
#include <userver/storages/clickhouse/io/columns/common_columns.hpp>
#include <userver/storages/clickhouse/io/columns/nullable_column.hpp>
#include <userver/storages/clickhouse/io/io_fwd.hpp>
#include <userver/storages/clickhouse/io/type_traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::impl {

template <typename T>
constexpr void EnsureInstantiationOfVector([[maybe_unused]] const T& t) {
  static_assert(meta::kIsInstantiationOf<std::vector, T>);
}

template <typename T>
struct EnsureInstantiationOfColumn {
  ~EnsureInstantiationOfColumn() {
    static_assert(!std::is_same_v<columns::ClickhouseColumn<T>, T>);
    static_assert(std::is_base_of_v<columns::ClickhouseColumn<T>, T>);
  }
};

template <typename T>
struct EnsureInstantiationOfColumn<columns::NullableColumn<T>> {
  ~EnsureInstantiationOfColumn() {
    static_assert(!std::is_same_v<columns::ClickhouseColumn<T>, T>);
    static_assert(std::is_base_of_v<columns::ClickhouseColumn<T>, T>);
  }
};

template <typename T,
          typename Seq = std::make_index_sequence<std::tuple_size_v<T>>>
struct TupleColumnsValidate;

template <typename T, size_t... S>
struct TupleColumnsValidate<T, std::index_sequence<S...>> {
  ~TupleColumnsValidate() {
    (...,
     (void)impl::EnsureInstantiationOfColumn<std::tuple_element_t<S, T>>{});
  }
};

template <size_t I, typename Row>
using CppType = boost::pfr::tuple_element_t<I, Row>;

template <typename T>
using MappedType = typename CppToClickhouse<T>::mapped_type;

template <size_t I, typename Row>
using ClickhouseType =
    typename std::tuple_element_t<I, MappedType<Row>>::cpp_type;

template <typename T>
inline constexpr auto kCppTypeColumnsCount = boost::pfr::tuple_size_v<T>;

template <typename T>
inline constexpr auto kClickhouseTypeColumnsCount =
    std::tuple_size_v<MappedType<T>>;

template <typename T>
constexpr void CommonValidateMapping() {
  static_assert(traits::kIsMappedToClickhouse<T>, "not mapped to clickhouse");
  static_assert(kCppTypeColumnsCount<T> == kClickhouseTypeColumnsCount<T>);

  [[maybe_unused]] TupleColumnsValidate<MappedType<T>> validator{};
}

template <typename T>
constexpr void ValidateColumnsMapping(const T& t) {
  boost::pfr::for_each_field(
      t, [](const auto& field) { impl::EnsureInstantiationOfVector(field); });

  impl::CommonValidateMapping<T>();
}

template <size_t I>
struct FailIndexAssertion : std::false_type {};

template <typename Row, size_t... I>
constexpr size_t FieldTypeFindMismatch(std::index_sequence<I...>) {
  constexpr bool results[] = {
      std::is_same_v<CppType<I, Row>, ClickhouseType<I, Row>>...};

  size_t i = 0;
  for (bool v : results) {
    if (!v) return i;
    ++i;
  }

  return i;
}

template <typename T>
constexpr void ValidateRowsMapping() {
  impl::CommonValidateMapping<T>();

  constexpr auto columns_count = kClickhouseTypeColumnsCount<T>;
  constexpr auto type_mismatch_index =
      FieldTypeFindMismatch<T>(std::make_index_sequence<columns_count>());
  if constexpr (type_mismatch_index != columns_count) {
    static_assert(std::is_same_v<CppType<type_mismatch_index, T>,
                                 ClickhouseType<type_mismatch_index, T>>,
                  "Make sure your ClickHouse mapping is correct.");
    static_assert(FailIndexAssertion<type_mismatch_index>::value,
                  "Recheck your mapping at this index.");
  }
}

template <typename T>
void ValidateRowsCount(const T& t) {
  std::optional<size_t> rows_count;
  boost::pfr::for_each_field(t, [&rows_count](const auto& field) {
    if (!rows_count.has_value()) {
      rows_count.emplace(field.size());
    }
    UINVARIANT(*rows_count == field.size(),
               "All rows should have same number of elements");
  });
}

template <typename T>
void ValidateColumnsCount(size_t expected) {
  constexpr auto columns_count = kCppTypeColumnsCount<T>;
  UINVARIANT(columns_count == expected, "Columns count mismatch.");
}

}  // namespace storages::clickhouse::io::impl

USERVER_NAMESPACE_END
