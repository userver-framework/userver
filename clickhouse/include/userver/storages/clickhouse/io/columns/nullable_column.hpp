#pragma once

/// @file userver/storages/clickhouse/io/columns/nullable_column.hpp
/// @brief Nullable column support
/// @ingroup userver_clickhouse_types

#include <optional>
#include <utility>

#include <userver/utils/assert.hpp>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>
#include <userver/storages/clickhouse/io/columns/common_columns.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

struct NullableColumnMeta final {
  ColumnRef nulls;
  ColumnRef inner;
};

NullableColumnMeta ExtractNullableMeta(const ColumnRef& column);

ColumnRef ConvertMetaToColumn(NullableColumnMeta&& meta);

/// @brief Represents ClickHouse Nullable(T) column,
/// where T is a ClickhouseColumn as well
template <typename T>
class NullableColumn final : public ClickhouseColumn<NullableColumn<T>> {
 public:
  using cpp_type = std::optional<typename T::cpp_type>;
  using container_type = std::vector<cpp_type>;

  class NullableDataHolder final {
   public:
    NullableDataHolder() = default;
    NullableDataHolder(typename ColumnIterator<
                           NullableColumn<T>>::IteratorPosition iter_position,
                       ColumnRef&& column);

    NullableDataHolder operator++(int);
    NullableDataHolder& operator++();
    void Next();
    cpp_type& UpdateValue();

    bool operator==(const NullableDataHolder& other) const;

   private:
    NullableDataHolder(typename ColumnIterator<
                           NullableColumn<T>>::IteratorPosition iter_position,
                       NullableColumnMeta&& meta);

    UInt8Column::iterator nulls_;
    typename T::iterator inner_;
    cpp_type current_value_ = std::nullopt;
    bool has_value_ = false;
  };
  using iterator_data = NullableDataHolder;

  NullableColumn(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

template <typename T>
NullableColumn<T>::NullableColumn(ColumnRef column)
    : ClickhouseColumn<NullableColumn>{column} {}

template <typename T>
NullableColumn<T>::NullableDataHolder::NullableDataHolder(
    typename ColumnIterator<NullableColumn<T>>::IteratorPosition iter_position,
    ColumnRef&& column)
    : NullableDataHolder(iter_position, ExtractNullableMeta(column)) {}

template <typename T>
NullableColumn<T>::NullableDataHolder::NullableDataHolder(
    typename ColumnIterator<NullableColumn<T>>::IteratorPosition iter_position,
    NullableColumnMeta&& meta)
    : nulls_{iter_position == decltype(iter_position)::kEnd
                 ? UInt8Column{meta.nulls}.end()
                 : UInt8Column{meta.nulls}.begin()},
      inner_{iter_position == decltype(iter_position)::kEnd
                 ? T{meta.inner}.end()
                 : T{meta.inner}.begin()} {}

template <typename T>
typename NullableColumn<T>::NullableDataHolder
NullableColumn<T>::NullableDataHolder::operator++(int) {
  NullableDataHolder old{};
  old.nulls_ = nulls_++;
  old.inner_ = inner_++;
  old.current_value_ = std::move_if_noexcept(current_value_);
  old.has_value_ = std::exchange(has_value_, false);

  return old;
}

template <typename T>
typename NullableColumn<T>::NullableDataHolder&
NullableColumn<T>::NullableDataHolder::operator++() {
  ++nulls_;
  ++inner_;
  current_value_.reset();
  has_value_ = false;

  return *this;
}

template <typename T>
typename NullableColumn<T>::cpp_type&
NullableColumn<T>::NullableDataHolder::UpdateValue() {
  if (!has_value_) {
    if (*nulls_) {
      current_value_.reset();
    } else {
      current_value_.emplace(std::move_if_noexcept(*inner_));
    }
    has_value_ = true;
  }

  return current_value_;
}

template <typename T>
bool NullableColumn<T>::NullableDataHolder::operator==(
    const NullableDataHolder& other) const {
  return nulls_ == other.nulls_ && inner_ == other.inner_;
}

template <typename T>
ColumnRef NullableColumn<T>::Serialize(const container_type& from) {
  typename T::container_type values;
  values.reserve(from.size());
  std::vector<uint8_t> nulls;
  nulls.reserve(from.size());

  for (auto& opt_v : from) {
    nulls.push_back(static_cast<uint8_t>(opt_v.has_value() ? 0 : 1));

    if (opt_v.has_value()) {
      values.push_back(*opt_v);
    } else {
      values.push_back(typename T::cpp_type{});
    }
  }

  NullableColumnMeta nullable_meta;
  nullable_meta.nulls = UInt8Column::Serialize(nulls);
  nullable_meta.inner = T::Serialize(values);
  return ConvertMetaToColumn(std::move(nullable_meta));
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
