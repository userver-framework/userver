#pragma once

#include <boost/pfr/precise.hpp>

#include <storages/postgres/result_set.hpp>

namespace storages::postgres::detail {

template <typename T>
struct IsTuple : std::false_type {};
template <typename... T>
struct IsTuple<std::tuple<T...>> : std::true_type {};

template <typename T, typename = ::utils::void_t<>>
struct HasIntrospection : std::false_type {};

template <typename T>
struct HasIntrospection<
    T, ::utils::void_t<decltype(std::declval<T&>().Introspect())>>
    : std::integral_constant<
          bool, IsTuple<decltype(std::declval<T&>().Introspect())>::value> {};

template <typename T>
constexpr bool DetectIsAggregate() {
  using type = typename std::remove_cv<T>::type;
  return std::is_class<type>::value && !std::is_empty<type>::value &&
         std::is_standard_layout<type>::value &&
         !std::is_polymorphic<type>::value && !std::is_union<type>::value;
}

template <typename T>
struct IsAggregateClass : std::integral_constant<bool, DetectIsAggregate<T>()> {
};

template <typename T>
constexpr bool kIsAggregateClass = IsAggregateClass<T>::value;

enum class RowTagType { kInvalid, kTuple, kAggregate, kIntrusiveIntrospection };

template <RowTagType Tag>
using RowTagConstant = std::integral_constant<RowTagType, Tag>;

template <typename T>
struct RowTag
    : std::conditional<
          IsTuple<T>::value, RowTagConstant<RowTagType::kTuple>,
          typename std::conditional<
              HasIntrospection<T>::value,
              RowTagConstant<RowTagType::kIntrusiveIntrospection>,
              typename std::conditional<
                  IsAggregateClass<T>::value,
                  RowTagConstant<RowTagType::kAggregate>,
                  RowTagConstant<RowTagType::kInvalid>>::type>::type>::type {};

template <typename T>
constexpr RowTagType kRowTag = RowTag<T>::value;

template <typename T>
struct RemoveTupleReferences;

template <typename... T>
struct RemoveTupleReferences<std::tuple<T...>> {
  using Type = std::tuple<typename std::remove_reference<T>::type...>;
};

template <typename T, RowTagType Tag>
struct RowToUserDataImpl;

template <typename T>
struct RowToUserDataImpl<T, RowTagType::kTuple> {
  using ValueType = T;

  static ValueType FromRow(const Row& row) { return row.AsTuple<ValueType>(); }
};

template <typename T>
struct RowToUserDataImpl<T, RowTagType::kAggregate> {
  using ValueType = T;
  using TupleType =
      decltype(boost::pfr::structure_tie(std::declval<ValueType&>()));
  using NonRefTuple = typename RemoveTupleReferences<TupleType>::Type;

  static TupleType Get(ValueType& v) { return boost::pfr::structure_tie(v); }
  static ValueType FromRow(const Row& row) {
    ValueType v;
    Get(v) = row.AsTuple<NonRefTuple>();
    return v;
  }
};

template <typename T>
struct RowToUserDataImpl<T, RowTagType::kIntrusiveIntrospection> {
  using ValueType = T;
  using TupleType = decltype(std::declval<ValueType&>().Introspect());
  using NonRefTuple = typename RemoveTupleReferences<TupleType>::Type;

  static TupleType Get(ValueType& v) { return v.Introspect(); }
  static ValueType FromRow(const Row& row) {
    ValueType v;
    Get(v) = row.AsTuple<NonRefTuple>();
    return v;
  }
};

template <typename T>
struct RowToUserData : RowToUserDataImpl<T, kRowTag<T>> {};

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
