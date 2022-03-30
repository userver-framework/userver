#pragma once

#include <iterator>
#include <optional>

#include <userver/storages/clickhouse/io/columns/column_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

template <typename ColumnType>
class BaseIterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = typename ColumnType::cpp_type;
  using reference = value_type&;
  using pointer = value_type*;

  BaseIterator() = default;
  BaseIterator(ColumnRef column);

  static BaseIterator End(ColumnRef column);

  BaseIterator operator++(int);
  BaseIterator& operator++();
  reference operator*() const;
  pointer operator->() const;

  bool operator==(const BaseIterator& other) const;
  bool operator!=(const BaseIterator& other) const;

  enum class IteratorPosition { kBegin, kEnd };

 private:
  BaseIterator(IteratorPosition iter_position, ColumnRef&& column);

  class DataHolder final {
   public:
    DataHolder() = default;
    DataHolder(IteratorPosition iter_position, ColumnRef&& column);

    void Next();
    value_type& UpdateValue();
    bool operator==(const DataHolder& other) const;

    value_type Get() const;

   private:
    ColumnRef column_;
    size_t ind_;

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
BaseIterator<ColumnType>::BaseIterator(ColumnRef column)
    : BaseIterator{IteratorPosition::kBegin, std::move(column)} {}

template <typename ColumnType>
BaseIterator<ColumnType> BaseIterator<ColumnType>::End(ColumnRef column) {
  return BaseIterator{IteratorPosition::kEnd, std::move(column)};
}

template <typename ColumnType>
BaseIterator<ColumnType>::BaseIterator(IteratorPosition iter_position,
                                       ColumnRef&& column)
    : data_{data_holder{iter_position, std::move(column)}} {}

template <typename ColumnType>
BaseIterator<ColumnType> BaseIterator<ColumnType>::operator++(int) {
  BaseIterator<ColumnType> old{*this};
  ++*this;

  return old;
}

template <typename ColumnType>
BaseIterator<ColumnType>& BaseIterator<ColumnType>::operator++() {
  data_.Next();
  return *this;
}

template <typename ColumnType>
typename BaseIterator<ColumnType>::reference
    BaseIterator<ColumnType>::operator*() const {
  return data_.UpdateValue();
}

template <typename ColumnType>
typename BaseIterator<ColumnType>::pointer
    BaseIterator<ColumnType>::operator->() const {
  return &data_.UpdateValue();
}

template <typename ColumnType>
bool BaseIterator<ColumnType>::operator==(const BaseIterator& other) const {
  return data_ == other.data_;
}

template <typename ColumnType>
bool BaseIterator<ColumnType>::operator!=(
    const BaseIterator<ColumnType>& other) const {
  return !((*this) == other);
}

template <typename ColumnType>
BaseIterator<ColumnType>::DataHolder::DataHolder(IteratorPosition iter_position,
                                                 ColumnRef&& column)
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
void BaseIterator<ColumnType>::DataHolder::Next() {
  ++ind_;
  current_value_.reset();
}

template <typename ColumnType>
typename BaseIterator<ColumnType>::value_type&
BaseIterator<ColumnType>::DataHolder::UpdateValue() {
  if (!current_value_.has_value()) {
    current_value_.emplace(Get());
  }
  return *current_value_;
}

template <typename ColumnType>
bool BaseIterator<ColumnType>::DataHolder::operator==(
    const DataHolder& other) const {
  return ind_ == other.ind_ && column_.get() == other.column_.get();
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
