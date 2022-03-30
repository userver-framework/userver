#pragma once

#include <type_traits>
#include <vector>

#include <boost/pfr/core.hpp>

#include <userver/utils/meta.hpp>

#include <userver/storages/clickhouse/io/columns/base_column.hpp>
#include <userver/storages/clickhouse/io/columns/common_columns.hpp>
#include <userver/storages/clickhouse/io/columns/nullable_column.hpp>
#include <userver/storages/clickhouse/io/io_fwd.hpp>
#include <userver/storages/clickhouse/io/type_traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io {

namespace impl {

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
          typename Seq = std::make_integer_sequence<int, std::tuple_size_v<T>>>
struct TupleColumnsValidate;

template <typename T, int... S>
struct TupleColumnsValidate<T, std::integer_sequence<int, S...>> {
  ~TupleColumnsValidate() {
    (..., (void)EnsureInstantiationOfColumn<std::tuple_element_t<S, T>>{});
  }
};

}  // namespace impl

template <typename T>
constexpr void Validate(const T& t) {
  boost::pfr::for_each_field(
      t, [](const auto& field) { impl::EnsureInstantiationOfVector(field); });

  static_assert(traits::kIsMappedToClickhouse<T>, "not mapped to clickhouse");
  static_assert(boost::pfr::tuple_size_v<T> ==
                std::tuple_size_v<typename CppToClickhouse<T>::mapped_type>);

  using mapped_type = typename CppToClickhouse<T>::mapped_type;
  [[maybe_unused]] impl::TupleColumnsValidate<mapped_type> validator{};
}

template <typename T>
void ValidateRowsCount(const T& t) {
  std::optional<size_t> rows_count;
  boost::pfr::for_each_field(t, [&rows_count](const auto& field) {
    if (!rows_count.has_value()) {
      rows_count.emplace(field.size());
    } else if (*rows_count != field.size()) {
      throw std::runtime_error{"rows count mismatch"};
    }
  });
}

}  // namespace storages::clickhouse::io

USERVER_NAMESPACE_END
