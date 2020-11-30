#pragma once

#include <boost/pfr/precise/core.hpp>

#include <storages/postgres/detail/is_in_namespace.hpp>
#include <storages/postgres/io/type_traits.hpp>

#include <utils/strong_typedef.hpp>

namespace storages::postgres {

struct RowTag {};
struct FieldTag {};
constexpr RowTag kRowTag;
constexpr FieldTag kFieldTag;

namespace io {

namespace traits {
//@{
/** @name Row type traits */
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

namespace detail {

template <typename T>
constexpr bool DetectIsSuitableRowType() {
  using type = std::remove_cv_t<T>;
  return std::is_class_v<type> && !std::is_empty_v<type> &&
         std::is_standard_layout_v<type> && std::is_aggregate_v<type> &&
         !std::is_polymorphic_v<type> && !std::is_union_v<type> &&
         !postgres::detail::IsInStdNamespace<type>::value &&
         !postgres::detail::IsInBoostNamespace<type>::value;
}

}  // namespace detail

template <typename T>
struct IsSuitableRowType : BoolConstant<detail::DetectIsSuitableRowType<T>()> {
};

template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
struct IsSuitableRowType<::utils::StrongTypedef<Tag, T, Ops, Enable>>
    : IsSuitableRowType<T> {};

template <typename T>
constexpr bool kIsSuitableRowType = IsSuitableRowType<T>::value;

enum class RowCategoryType {
  kNonRow,
  kTuple,
  kAggregate,
  kIntrusiveIntrospection
};

template <RowCategoryType Tag>
using RowCategoryConstant = std::integral_constant<RowCategoryType, Tag>;

template <typename T>
struct RowCategory
    : std::conditional_t<
          IsTuple<T>::value, RowCategoryConstant<RowCategoryType::kTuple>,
          std::conditional_t<
              HasIntrospection<T>::value,
              RowCategoryConstant<RowCategoryType::kIntrusiveIntrospection>,
              std::conditional_t<
                  IsSuitableRowType<T>::value,
                  RowCategoryConstant<RowCategoryType::kAggregate>,
                  RowCategoryConstant<RowCategoryType::kNonRow>>>> {};

template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
struct RowCategory<::utils::StrongTypedef<Tag, T, Ops, Enable>>
    : RowCategory<T> {};

template <typename T>
constexpr RowCategoryType kRowCategory = RowCategory<T>::value;

template <typename T>
constexpr bool kIsRowType = kRowCategory<T> != RowCategoryType::kNonRow;

template <typename T>
constexpr bool kIsCompositeType = kIsRowType<T>;

template <typename T>
constexpr bool kIsColumnType = kRowCategory<T> == RowCategoryType::kNonRow;

template <typename T, typename Enable = ::utils::void_t<>>
struct ExtractionTag {
  using type = FieldTag;
};

template <typename T>
struct ExtractionTag<T, std::enable_if_t<kIsRowType<T>>> {
  using type = RowTag;
};

template <typename T>
constexpr typename ExtractionTag<T>::type kExtractionTag{};

}  // namespace traits

namespace detail {

template <typename T, traits::RowCategoryType C>
struct RowTypeImpl {
  static_assert(traits::kRowCategory<T> != traits::RowCategoryType::kNonRow,
                "This type cannot be used as a row type");
};

template <typename T>
struct RowTypeImpl<T, traits::RowCategoryType::kTuple> {
  using ValueType = T;
  using TupleType = T;
  static constexpr std::size_t size = std::tuple_size<TupleType>::value;
  using IndexSequence = std::make_index_sequence<size>;

  static TupleType& GetTuple(ValueType& v) { return v; }
  static const TupleType& GetTuple(const ValueType& v) { return v; }
};

template <typename T>
struct RowTypeImpl<T, traits::RowCategoryType::kAggregate> {
  using ValueType = T;
  using TupleType =
      decltype(boost::pfr::structure_tie(std::declval<ValueType&>()));
  static constexpr std::size_t size = std::tuple_size<TupleType>::value;

  using IndexSequence = std::make_index_sequence<size>;
  static TupleType GetTuple(ValueType& v) {
    return boost::pfr::structure_tie(v);
  }
  static auto GetTuple(const ValueType& value) {
    return boost::pfr::structure_to_tuple(value);
  }
};

template <typename T>
struct RowTypeImpl<T, traits::RowCategoryType::kIntrusiveIntrospection> {
  using ValueType = T;
  using TupleType = decltype(std::declval<ValueType&>().Introspect());
  static constexpr std::size_t size = std::tuple_size<TupleType>::value;
  using IndexSequence = std::make_index_sequence<size>;
  using ConstRefTuple = typename traits::AddTupleConstRef<TupleType>::type;

  static TupleType GetTuple(ValueType& v) { return v.Introspect(); }
  static auto GetTuple(const ValueType& v) {
    // const_cast here is to relieve users from burden of writing
    // const-overloaded functions or static template Introspect functions.
    return ConstRefTuple{const_cast<ValueType&>(v).Introspect()};
  }
};

}  // namespace detail

template <typename T>
struct RowType : detail::RowTypeImpl<T, traits::kRowCategory<T>> {};

}  // namespace io
}  // namespace storages::postgres
