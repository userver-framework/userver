#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include <userver/storages/clickhouse/impl/block_wrapper_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

template <typename TypesTuple>
struct IteratorsTuple;

template <typename... Ts>
struct IteratorsTuple<std::tuple<Ts...>> {
  using type = std::tuple<typename Ts::iterator...>;
};

template <typename T>
using IteratorsTupleT = typename IteratorsTuple<T>::type;

template <typename T,
          typename Seq = std::make_index_sequence<std::tuple_size_v<T>>>
class IteratorsHelper;

template <typename T, size_t... I>
class IteratorsHelper<T, std::index_sequence<I...>> {
 public:
  static void Init(IteratorsTupleT<T>& begin_iterators,
                   IteratorsTupleT<T>& end_iterators,
                   clickhouse::impl::BlockWrapper& block) {
    (..., (std::get<I>(begin_iterators) = GetBegin<I>(block)));
    (..., (std::get<I>(end_iterators) = GetEnd<I>(block)));
  }

  static void PrefixIncrement(IteratorsTupleT<T>& iterators) {
    (..., ++std::get<I>(iterators));
  }

  static IteratorsTupleT<T> PostfixIncrement(IteratorsTupleT<T>& iterators) {
    return IteratorsTupleT<T>{std::get<I>(iterators)++...};
  }

 private:
  template <size_t Index>
  static auto GetBegin(clickhouse::impl::BlockWrapper& block) {
    using ColumnType = std::tuple_element_t<Index, T>;
    return ColumnType{io::columns::GetWrappedColumn(block, Index)}.begin();
  }

  template <size_t Index>
  static auto GetEnd(clickhouse::impl::BlockWrapper& block) {
    using ColumnType = std::tuple_element_t<Index, T>;
    return ColumnType{io::columns::GetWrappedColumn(block, Index)}.end();
  }
};

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
