#pragma once

#include <boost/pfr/core.hpp>
#include <boost/pfr/traits.hpp>

#include <userver/storages/postgres/detail/is_in_namespace.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @brief Tag type to disambiguate reading the row to a user's row type
/// (values of the row initialize user's type data members).
///
/// @snippet storages/postgres/tests/typed_rows_pgtest.cpp RowTagSippet
struct RowTag {};

/// @brief Tag type to disambiguate reading the first value of a row to a
/// user's composite type (PostgreSQL composite type in the row initializes
/// user's type).
///
/// @snippet storages/postgres/tests/composite_types_pgtest.cpp FieldTagSippet
struct FieldTag {};

inline constexpr RowTag kRowTag{};
inline constexpr FieldTag kFieldTag{};

namespace io {

namespace traits {
//@{
/** @name Row type traits */
template <typename T>
struct IsTuple : std::false_type {};
template <typename... T>
struct IsTuple<std::tuple<T...>> : std::true_type {};

namespace impl {

template <typename T>
struct HasConstIntrospection : std::false_type {};

template <typename T>
requires requires(const T& t) { t.Introspect(); }
struct HasConstIntrospection<T> : std::true_type {};

template <typename T>
struct HasNonConstIntrospection : std::false_type {
    static_assert(
        !impl::HasConstIntrospection<T>::value,
        "PostgreSQL driver requires non-const Introspect(). "
        "Example: auto Introspect() { return std::tie(a, b, c, d); }"
    );
};

template <typename T>
requires requires(T& t) { t.Introspect(); }
struct HasNonConstIntrospection<T> : std::true_type {
    static_assert(
        IsTuple<decltype(std::declval<T&>().Introspect())>::value,
        "Introspect() should return a std::tuple. "
        "Example: auto Introspect() { return std::tie(a, b, c, d); }"
    );
};

}  // namespace impl

template <typename T>
struct HasIntrospection : impl::HasNonConstIntrospection<T> {
    static_assert(!std::is_const_v<T>);
    static_assert(!std::is_reference_v<T>);
};

namespace detail {

struct ForDeserializationTag;

template <typename T>
struct IsPostgresBuildInTypeWrapperImpl : std::false_type {};

template <typename T>
requires requires { T::kIsPostgresBuildInTypeWrapper; }
struct IsPostgresBuildInTypeWrapperImpl<T> : std::integral_constant<const bool, T::kIsPostgresBuildInTypeWrapper> {
    static_assert(
        std::is_same_v<decltype(T::kIsPostgresBuildInTypeWrapper), const bool>,
        "kIsPostgresBuildInTypeWrapper must be bool"
    );
};

template <typename T>
concept IsPostgresBuildInTypeWrapper = IsPostgresBuildInTypeWrapperImpl<T>::value;

template <typename T>
constexpr bool DetectIsSuitableRowType() {
    using type = std::remove_cv_t<T>;
    return std::is_class_v<type> && !std::is_empty_v<type> &&
           boost::pfr::is_implicitly_reflectable_v<type, detail::ForDeserializationTag> &&
           !std::is_polymorphic_v<type> && !std::is_union_v<type> && !postgres::detail::IsInStdNamespace<type> &&
           !postgres::detail::IsInBoostNamespace<type> && !detail::IsPostgresBuildInTypeWrapper<type>;
}

template <typename T>
struct IsSuitableRowType : BoolConstant<detail::DetectIsSuitableRowType<T>()> {};

template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
struct IsSuitableRowType<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>> : IsSuitableRowType<T> {};

}  // namespace detail

enum class RowCategoryType { kNonRow, kTuple, kAggregate, kIntrusiveIntrospection };

template <RowCategoryType Tag>
using RowCategoryConstant = std::integral_constant<RowCategoryType, Tag>;

template <typename T>
struct RowCategory
    : std::conditional_t<
          IsTuple<T>::value,
          RowCategoryConstant<RowCategoryType::kTuple>,
          std::conditional_t<
              HasIntrospection<T>::value,
              RowCategoryConstant<RowCategoryType::kIntrusiveIntrospection>,
              std::conditional_t<
                  detail::IsSuitableRowType<T>::value,
                  RowCategoryConstant<RowCategoryType::kAggregate>,
                  RowCategoryConstant<RowCategoryType::kNonRow>>>> {};

template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
struct RowCategory<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>> : RowCategory<T> {};

template <typename T>
inline constexpr RowCategoryType kRowCategory = RowCategory<T>::value;

template <typename T>
constexpr void AssertIsValidRowType() {
    static_assert(
        kRowCategory<T> != RowCategoryType::kNonRow,
        "Row type must be one of the following: "
        "1. primitive type. "
        "2. std::tuple. "
        "3. Aggregation type. See std::aggregation. "
        "4. Has a Introspect method that makes the std::tuple from your class/struct. "
        "For more info see `uPg: Typed PostgreSQL results` chapter in docs."
    );
}

template <typename T>
concept IsRowType = kRowCategory<T> != RowCategoryType::kNonRow;

template <typename T>
concept IsCompositeType = IsRowType<T>;

template <typename T>
concept IsColumnType = kRowCategory<T> == RowCategoryType::kNonRow;

// NOLINTBEGIN(readability-identifier-naming)

/// @deprecated
template <typename T>
concept kIsRowType = IsRowType<T>;

/// @deprecated
template <typename T>
concept kIsCompositeType = IsCompositeType<T>;

/// @deprecated
template <typename T>
concept kIsColumnType = IsColumnType<T>;

// NOLINTEND(readability-identifier-naming)

template <typename T>
struct ExtractionTag {
    using type = FieldTag;
};

template <typename T>
requires IsRowType<T>
struct ExtractionTag<T> {
    using type = RowTag;
};

}  // namespace traits

namespace detail {

template <typename T, traits::RowCategoryType C>
struct RowTypeImpl {
    static_assert(
        traits::kRowCategory<T> != traits::RowCategoryType::kNonRow,
        "This type cannot be used as a row type"
    );
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
    using TupleType = decltype(boost::pfr::structure_tie(std::declval<ValueType&>()));
    static constexpr std::size_t size = std::tuple_size<TupleType>::value;

    using IndexSequence = std::make_index_sequence<size>;
    static TupleType GetTuple(ValueType& v) { return boost::pfr::structure_tie(v); }
    static auto GetTuple(const ValueType& value) { return boost::pfr::structure_to_tuple(value); }
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
        /// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        return ConstRefTuple{const_cast<ValueType&>(v).Introspect()};
    }
};

}  // namespace detail

template <typename T>
struct RowType : detail::RowTypeImpl<T, traits::kRowCategory<T>> {};

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END
