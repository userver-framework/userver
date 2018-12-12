#pragma once

#include <boost/pfr/precise.hpp>

#include <storages/postgres/result_set.hpp>

namespace storages::postgres::detail {

template <typename T, io::traits::RowTagType Tag>
struct RowToUserDataImpl;

template <typename T>
struct RowToUserDataImpl<T, io::traits::RowTagType::kTuple> {
  using ValueType = T;

  static ValueType FromRow(const Row& row) { return row.AsTuple<ValueType>(); }
};

template <typename T>
struct RowToUserDataImpl<T, io::traits::RowTagType::kAggregate> {
  using ValueType = T;
  using TupleType =
      decltype(boost::pfr::structure_tie(std::declval<ValueType&>()));
  using NonRefTuple =
      typename io::traits::RemoveTupleReferences<TupleType>::type;

  static TupleType Get(ValueType& v) { return boost::pfr::structure_tie(v); }
  static ValueType FromRow(const Row& row) {
    ValueType v;
    Get(v) = row.AsTuple<NonRefTuple>();
    return v;
  }
};

template <typename T>
struct RowToUserDataImpl<T, io::traits::RowTagType::kIntrusiveIntrospection> {
  using ValueType = T;
  using TupleType = decltype(std::declval<ValueType&>().Introspect());
  using NonRefTuple =
      typename io::traits::RemoveTupleReferences<TupleType>::type;

  static TupleType Get(ValueType& v) { return v.Introspect(); }
  static ValueType FromRow(const Row& row) {
    ValueType v;
    Get(v) = row.AsTuple<NonRefTuple>();
    return v;
  }
};

template <typename T>
struct RowToUserData : RowToUserDataImpl<T, io::traits::kRowTag<T>> {};

template <typename T>
class ConstTypedRowIterator : private Row {
 public:
  //@{
  /** @name Iterator concept */
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = value_type;
  using pointer = const value_type*;
  /// This iterator doesn't satisfy ForwardIterator concept as it returns
  /// data by value. In all other respects it behaves as a random access
  /// iterator.
  using iterator_category = std::input_iterator_tag;
  //@}
 private:
  //@{
  using UserData = RowToUserData<T>;
  //@}
 public:
  //@{
  /** @name Iterator dereferencing */
  /// Read typed value from underlying postgres buffers and return it.
  /// Please note there is no operator ->.
  value_type operator*() const { return UserData::FromRow(*this); }
  //@}
  //@{
  /** @name Iterator validity */
  explicit operator bool() const { return this->IsValid(); }
  bool operator!() const { return !this->IsValid(); }
  //@}

  //@{
  /** @name Iterator movement */
  ConstTypedRowIterator& operator++() { return DoAdvance(1); }
  ConstTypedRowIterator operator++(int) {
    ConstTypedRowIterator prev{*this};
    DoAdvance(1);
    return prev;
  }

  ConstTypedRowIterator& operator--() { return DoAdvance(-1); }
  ConstTypedRowIterator operator--(int) {
    ConstTypedRowIterator prev{*this};
    DoAdvance(-1);
    return prev;
  }

  ConstTypedRowIterator operator+(difference_type distance) const {
    ConstTypedRowIterator res{*this};
    res.DoAdvance(distance);
    return res;
  }
  ConstTypedRowIterator operator-(difference_type distance) const {
    ConstTypedRowIterator res{*this};
    res.DoAdvance(-distance);
    return res;
  }

  difference_type operator-(const ConstTypedRowIterator& rhs) const {
    return this->Distance(rhs);
  }

  ConstTypedRowIterator operator[](difference_type index) const {
    return *this + index;
  }

  ConstTypedRowIterator& operator+=(difference_type distance) {
    return DoAdvance(distance);
  }
  ConstTypedRowIterator& operator-=(difference_type distance) {
    return DoAdvance(-distance);
  }
  //@}

  //@{
  /** @name Iterator comparison */
  bool operator==(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) == 0;
  }

  bool operator!=(const ConstTypedRowIterator& rhs) const {
    return !(*this == rhs);
  }

  bool operator<(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) < 0;
  }
  bool operator<=(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) <= 0;
  }
  bool operator>(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) > 0;
  }
  bool operator>=(const ConstTypedRowIterator& rhs) const {
    return this->Compare(rhs) >= 0;
  }
  //@}
 private:
  friend class TypedResultSet<T>;

  ConstTypedRowIterator(detail::ResultWrapperPtr res, size_type row)
      : Row(res, row) {}
  ConstTypedRowIterator& DoAdvance(difference_type distance) {
    this->Advance(distance);
    return *this;
  }
};

}  // namespace storages::postgres::detail
