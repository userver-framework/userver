#pragma once

/// @file userver/storages/clickhouse/io/columns/column_iterator.hpp
/// @brief @copybrief storages::clickhouse::io::columns::BaseIterator

#include <iterator>
#include <optional>

#include <userver/utils/assert.hpp>

#include <userver/storages/clickhouse/io/columns/column_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io {

class IteratorsTester;

namespace columns {

/// @brief Forward-iterator for iterating over column of type ColumnType
template <typename ColumnType>
class ColumnIterator final {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = typename ColumnType::cpp_type;
  using reference = value_type&;
  using pointer = value_type*;

  ColumnIterator() = default;
  ColumnIterator(ColumnRef column);

  static ColumnIterator End(ColumnRef column);

  ColumnIterator operator++(int);
  ColumnIterator& operator++();
  reference operator*() const;
  pointer operator->() const;

  bool operator==(const ColumnIterator& other) const;
  bool operator!=(const ColumnIterator& other) const;

  enum class IteratorPosition { kBegin, kEnd };

  friend class storages::clickhouse::io::IteratorsTester;

 private:
  ColumnIterator(IteratorPosition iter_position, ColumnRef&& column);

  class DataHolder final {
   public:
    DataHolder() = default;
    DataHolder(IteratorPosition iter_position, ColumnRef&& column);

    DataHolder operator++(int);
    DataHolder& operator++();
    value_type& UpdateValue();
    bool operator==(const DataHolder& other) const;

    value_type Get() const;

    friend class storages::clickhouse::io::IteratorsTester;

   private:
    ColumnRef column_;
    size_t ind_{0};

    std::optional<value_type> current_value_ = std::nullopt;
  };

  template <typename T, typename = void>
  struct IteratorDataHolder final {
    using type = DataHolder;
  };

  template <typename T>
  struct IteratorDataHolder<T, std::void_t<typename T::iterator_data>> final {
    using type = typename T::iterator_data;
  };

  using data_holder = typename IteratorDataHolder<ColumnType>::type;

  mutable data_holder data_;
};

template <typename ColumnType>
ColumnIterator<ColumnType>::ColumnIterator(ColumnRef column)
    : ColumnIterator{IteratorPosition::kBegin, std::move(column)} {}

template <typename ColumnType>
ColumnIterator<ColumnType> ColumnIterator<ColumnType>::End(ColumnRef column) {
  return ColumnIterator{IteratorPosition::kEnd, std::move(column)};
}

template <typename ColumnType>
ColumnIterator<ColumnType>::ColumnIterator(IteratorPosition iter_position,
                                           ColumnRef&& column)
    : data_{data_holder{iter_position, std::move(column)}} {}

template <typename ColumnType>
ColumnIterator<ColumnType> ColumnIterator<ColumnType>::operator++(int) {
  ColumnIterator<ColumnType> old{};
  old.data_ = data_++;

  return old;
}

template <typename ColumnType>
ColumnIterator<ColumnType>& ColumnIterator<ColumnType>::operator++() {
  ++data_;
  return *this;
}

template <typename ColumnType>
typename ColumnIterator<ColumnType>::reference
ColumnIterator<ColumnType>::operator*() const {
  return data_.UpdateValue();
}

template <typename ColumnType>
typename ColumnIterator<ColumnType>::pointer
ColumnIterator<ColumnType>::operator->() const {
  return &data_.UpdateValue();
}

template <typename ColumnType>
bool ColumnIterator<ColumnType>::operator==(const ColumnIterator& other) const {
  return data_ == other.data_;
}

template <typename ColumnType>
bool ColumnIterator<ColumnType>::operator!=(
    const ColumnIterator<ColumnType>& other) const {
  return !((*this) == other);
}

template <typename ColumnType>
ColumnIterator<ColumnType>::DataHolder::DataHolder(
    IteratorPosition iter_position, ColumnRef&& column)
    : column_{std::move(column)} {
  switch (iter_position) {
    case IteratorPosition::kBegin: {
      ind_ = 0;
      break;
    }
    case IteratorPosition::kEnd: {
      ind_ = GetColumnSize(column_);
      break;
    }
  }
}

template <typename ColumnType>
typename ColumnIterator<ColumnType>::DataHolder
ColumnIterator<ColumnType>::DataHolder::operator++(int) {
  DataHolder old{};
  old.column_ = column_;
  old.ind_ = ind_++;
  old.current_value_ = std::move_if_noexcept(current_value_);
  current_value_.reset();

  return old;
}

template <typename ColumnType>
typename ColumnIterator<ColumnType>::DataHolder&
ColumnIterator<ColumnType>::DataHolder::operator++() {
  ++ind_;
  current_value_.reset();

  return *this;
}

template <typename ColumnType>
typename ColumnIterator<ColumnType>::value_type&
ColumnIterator<ColumnType>::DataHolder::UpdateValue() {
  UASSERT(ind_ < GetColumnSize(column_));
  if (!current_value_.has_value()) {
    current_value_.emplace(Get());
  }
  return *current_value_;
}

template <typename ColumnType>
bool ColumnIterator<ColumnType>::DataHolder::operator==(
    const DataHolder& other) const {
  return ind_ == other.ind_ && column_.get() == other.column_.get();
}

}  // namespace columns

}  // namespace storages::clickhouse::io

USERVER_NAMESPACE_END
