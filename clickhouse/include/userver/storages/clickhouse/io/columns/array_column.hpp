#pragma once

/// @file userver/storages/clickhouse/io/columns/array_column.hpp
/// @brief Array column support
/// @ingroup userver_clickhouse_types

#include <userver/utils/assert.hpp>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>
#include <userver/storages/clickhouse/io/columns/common_columns.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

struct ArrayColumnMeta final {
  ColumnRef data;
  ColumnRef offsets;
};

ColumnRef ConvertMetaToColumn(ArrayColumnMeta&& meta);
ColumnRef ExtractArrayItem(const ColumnRef& column, std::size_t ind);

/// @brief Represents ClickHouse Array(T) column,
/// where T is a ClickhouseColumn as well
template <typename T>
class ArrayColumn final : public ClickhouseColumn<ArrayColumn<T>> {
 public:
  using cpp_type = std::vector<typename T::cpp_type>;
  using container_type = std::vector<cpp_type>;

  ArrayColumn(ColumnRef column);

  class ArrayDataHolder final {
   public:
    ArrayDataHolder() = default;
    ArrayDataHolder(
        typename ColumnIterator<ArrayColumn<T>>::IteratorPosition iter_position,
        ColumnRef&& column);

    ArrayDataHolder operator++(int);
    ArrayDataHolder& operator++();
    cpp_type& UpdateValue();

    bool operator==(const ArrayDataHolder& other) const;

   private:
    ColumnRef inner_{};
    std::size_t ind_{0};
    std::optional<cpp_type> current_value_ = std::nullopt;
  };
  using iterator_data = ArrayDataHolder;

  static ColumnRef Serialize(const container_type& from);
  static cpp_type RetrieveElement(const ColumnRef& ref, std::size_t ind);
};

template <typename T>
ArrayColumn<T>::ArrayDataHolder::ArrayDataHolder(
    typename ColumnIterator<ArrayColumn<T>>::IteratorPosition iter_position,
    ColumnRef&& column)
    : inner_{std::move(column)},
      ind_(iter_position == decltype(iter_position)::kEnd
               ? GetColumnSize(inner_)
               : 0) {}

template <typename T>
typename ArrayColumn<T>::ArrayDataHolder
ArrayColumn<T>::ArrayDataHolder::operator++(int) {
  ArrayDataHolder old{};
  old.inner_ = inner_;
  old.ind_ = ind_++;
  old.current_value_ = std::move_if_noexcept(current_value_);
  current_value_.reset();

  return old;
}

template <typename T>
typename ArrayColumn<T>::ArrayDataHolder&
ArrayColumn<T>::ArrayDataHolder::operator++() {
  ++ind_;
  current_value_.reset();

  return *this;
}

template <typename T>
typename ArrayColumn<T>::cpp_type&
ArrayColumn<T>::ArrayDataHolder::UpdateValue() {
  UASSERT(ind_ < GetColumnSize(inner_));
  if (!current_value_.has_value()) {
    cpp_type item = RetrieveElement(inner_, ind_);
    current_value_.emplace(std::move(item));
  }

  return *current_value_;
}

template <typename T>
bool ArrayColumn<T>::ArrayDataHolder::operator==(
    const ArrayDataHolder& other) const {
  return inner_.get() == other.inner_.get() && ind_ == other.ind_;
}

template <typename T>
ArrayColumn<T>::ArrayColumn(ColumnRef column)
    : ClickhouseColumn<ArrayColumn>{column} {}

template <typename T>
ColumnRef ArrayColumn<T>::Serialize(const container_type& from) {
  uint64_t cumulative_offset = 0;
  std::vector<uint64_t> offsets;
  offsets.reserve(from.size());

  for (const auto& value : from) {
    cumulative_offset += value.size();
    offsets.push_back(cumulative_offset);
  }
  typename T::container_type values;
  values.reserve(cumulative_offset);

  for (const auto& value : from) {
    for (const auto& item : value) {
      values.push_back(item);
    }
  }

  ArrayColumnMeta array_meta;
  array_meta.offsets = UInt64Column::Serialize(offsets);
  array_meta.data = T::Serialize(values);

  return ConvertMetaToColumn(std::move(array_meta));
}

template <typename T>
typename ArrayColumn<T>::cpp_type ArrayColumn<T>::RetrieveElement(
    const ColumnRef& ref, std::size_t ind) {
  auto array_item = ExtractArrayItem(ref, ind);
  T typed_column(array_item);

  cpp_type result;
  result.reserve(GetColumnSize(array_item));
  for (auto it = typed_column.begin(); it != typed_column.end(); ++it) {
    result.push_back(std::move(*it));
  }
  return result;
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
