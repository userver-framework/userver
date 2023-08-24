#pragma once

#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include <boost/pfr/core.hpp>

#include <userver/storages/clickhouse/impl/block_wrapper_fwd.hpp>
#include <userver/storages/clickhouse/impl/iterators_helper.hpp>
#include <userver/storages/clickhouse/io/columns/column_wrapper.hpp>
#include <userver/storages/clickhouse/io/io_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io {

class IteratorsTester;

template <typename MappedType>
class ColumnsMapper final {
 public:
  explicit ColumnsMapper(clickhouse::impl::BlockWrapper& block)
      : block_{block} {}

  template <typename Field, size_t Index>
  void operator()(Field& field, std::integral_constant<size_t, Index> i) {
    using ColumnType = std::tuple_element_t<Index, MappedType>;
    static_assert(std::is_same_v<Field, typename ColumnType::container_type>);

    auto column = ColumnType{io::columns::GetWrappedColumn(block_, i)};
    field.reserve(column.Size());
    for (auto& it : column) field.push_back(std::move(it));
  }

 private:
  clickhouse::impl::BlockWrapper& block_;
};

template <typename Row>
class RowsMapper final {
 public:
  using MappedType = typename CppToClickhouse<Row>::mapped_type;
  explicit RowsMapper(clickhouse::impl::BlockWrapperPtr&& block)
      : block_{block.release()} {
    IteratorsHelperT::Init(begin_iterators_, end_iterators_, *block_);
  }
  ~RowsMapper() = default;

  using IteratorsTupleT = clickhouse::impl::IteratorsTupleT<MappedType>;

  class Iterator final {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Row;
    using reference = value_type&;
    using pointer = value_type*;

    Iterator() = default;
    Iterator(IteratorsTupleT iterators);

    Iterator operator++(int);
    Iterator& operator++();
    reference operator*() const;
    pointer operator->() const;

    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;

    friend class IteratorsTester;

   private:
    Row& UpdateValue() const;

    class FieldMapper final {
     public:
      FieldMapper(const IteratorsTupleT& iterators) : iterators_{iterators} {}

      template <typename Field, size_t Index>
      void operator()(
          Field& field,
          [[maybe_unused]] std::integral_constant<size_t, Index> i) const {
        if constexpr (kCanMoveFromIterators) {
          field = std::move(*std::get<Index>(iterators_));
        } else {
          field = *std::get<Index>(iterators_);
        }
      }

     private:
      const IteratorsTupleT& iterators_;
    };

    IteratorsTupleT iterators_;
    mutable std::optional<Row> current_value_ = std::nullopt;
  };

  Iterator begin() const { return Iterator{begin_iterators_}; }
  Iterator end() const { return Iterator{end_iterators_}; }

  friend class IteratorsTester;

 private:
  template <typename TypesTuple>
  struct IteratorValuesAreNoexceptMovable;

  template <typename... Ts>
  struct IteratorValuesAreNoexceptMovable<std::tuple<Ts...>>
      : std::conjunction<std::is_nothrow_move_constructible<
            typename Ts::iterator::value_type>...> {};

  static constexpr auto kCanMoveFromIterators =
      IteratorValuesAreNoexceptMovable<MappedType>::value;

  using IteratorsHelperT = clickhouse::impl::IteratorsHelper<MappedType>;

  clickhouse::impl::BlockWrapperPtr block_;
  IteratorsTupleT begin_iterators_{};
  IteratorsTupleT end_iterators_{};
};

template <typename Row>
RowsMapper<Row>::Iterator::Iterator(IteratorsTupleT iterators)
    : iterators_{std::move(iterators)} {}

template <typename Row>
typename RowsMapper<Row>::Iterator RowsMapper<Row>::Iterator::operator++(int) {
  Iterator old{};
  old.iterators_ = IteratorsHelperT::PostfixIncrement(iterators_);
  old.current_value_ = std::move_if_noexcept(current_value_);
  current_value_.reset();

  return old;
}

template <typename Row>
typename RowsMapper<Row>::Iterator& RowsMapper<Row>::Iterator::operator++() {
  IteratorsHelperT::PrefixIncrement(iterators_);
  current_value_.reset();

  return *this;
}

template <typename Row>
typename RowsMapper<Row>::Iterator::reference
RowsMapper<Row>::Iterator::operator*() const {
  return UpdateValue();
}

template <typename Row>
typename RowsMapper<Row>::Iterator::pointer
RowsMapper<Row>::Iterator::operator->() const {
  return &UpdateValue();
}

template <typename Row>
Row& RowsMapper<Row>::Iterator::UpdateValue() const {
  if (!current_value_.has_value()) {
    Row result{};
    boost::pfr::for_each_field(result, FieldMapper{iterators_});
    current_value_.emplace(std::move(result));
  }

  return *current_value_;
}

template <typename Row>
bool RowsMapper<Row>::Iterator::operator==(const Iterator& other) const {
  return iterators_ == other.iterators_;
}

template <typename Row>
bool RowsMapper<Row>::Iterator::operator!=(const Iterator& other) const {
  return !((*this) == other);
}

}  // namespace storages::clickhouse::io

USERVER_NAMESPACE_END
