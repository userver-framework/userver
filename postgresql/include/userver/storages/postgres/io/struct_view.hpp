#pragma once

/// @file userver/storages/postgres/io/struct_view.hpp
/// @brief Reading and writing a subset of a structure's members as a row
/// @ingroup userver_postgres_parse_and_format

#include <concepts>
#include <tuple>
#include <type_traits>

#include <boost/pfr.hpp>

#include <userver/storages/postgres/io/row_types.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

namespace traits {

/// The class a pointer-to-member belongs to, so the pack can be checked against
/// `T` rather than merely against "is some member pointer".
template <typename M>
struct MemberOwner;

template <typename C, typename F>
struct MemberOwner<F C::*> {
    using type = C;
};

template <auto M>
using MemberOwnerT = typename MemberOwner<std::remove_cv_t<decltype(M)>>::type;

template <typename T, auto... Members>
constexpr bool kAllMembersOf =
    (... && (std::is_member_object_pointer_v<decltype(Members)> && std::is_base_of_v<MemberOwnerT<Members>, T>));

}  // namespace traits

namespace detail {

template <auto... Members>
auto PartialTie(auto&& value) {
    using OwnerType = std::remove_cvref_t<decltype(value)>;
    if constexpr (sizeof...(Members) > 0) {
        static_assert(traits::kAllMembersOf<OwnerType, Members...>, "All members must be members of the same class");
        return std::tie(std::forward<decltype(value)>(value).*Members...);
    } else if constexpr (traits::kRowCategory<OwnerType> == traits::RowCategoryType::kIntrusiveIntrospection) {
        return RowType<OwnerType>::GetTuple(std::forward<decltype(value)>(value));
    } else {
        return boost::pfr::structure_tie(std::forward<decltype(value)>(value));
    }
}

}  // namespace detail

/// @brief Structure view type
/// @details An adapter to read/write a subset of the data members of a struct
/// from a PostgreSQL row.
///   When reading it doesn't hold any data and is used solely as a static
///   discriminator for reading.
/// @tparam T The type of the struct
/// @tparam Members The members of the struct to be parsed
template <typename T, auto... Members>
struct StructView {
    using UnderlyingType = std::remove_cvref_t<T>;

    template <typename U>
    requires std::derived_from<std::remove_cvref_t<U>, UnderlyingType>
    static auto Tie(U& value) {
        using UnderlyingRef = std::conditional_t<std::is_const_v<U>, const UnderlyingType&, UnderlyingType&>;
        return detail::PartialTie<Members...>(static_cast<UnderlyingRef>(value));
    }

    static constexpr auto size = std::tuple_size_v<decltype(Tie(std::declval<const UnderlyingType&>()))>;

    StructView() = delete;
    StructView(StructView&&) = default;
    StructView(const StructView&) = default;

    StructView(const UnderlyingType& val) : value_(&val) {}

    auto Params() const { return Tie(*value_); }

private:
    const UnderlyingType* value_{nullptr};
};

namespace traits {

template <typename T>
struct IsStructView : std::false_type {};

template <typename T, auto... Members>
struct IsStructView<StructView<T, Members...>> : std::true_type {};

template <typename T>
concept RequiresStructView = IsStructView<T>::value;

template <typename T>
struct RowExtraction {
    using type = T;
};

template <typename T, auto... Members>
struct RowExtraction<StructView<T, Members...>> {
    using type = typename StructView<T, Members...>::UnderlyingType;
};

/// The type a row read as `T` yields: `T` itself, or the viewed structure for a StructView.
template <typename T>
using RowExtractionT = typename RowExtraction<T>::type;

template <typename Container>
struct ContainerRowExtraction {
    using type = Container;
};

template <typename Container>
requires IsStructView<std::remove_cvref_t<typename Container::value_type>>::value
struct ContainerRowExtraction<Container> {
    using type = meta::RebindContainer<Container, RowExtractionT<std::remove_cvref_t<typename Container::value_type>>>;
};

/// The container a result read as `Container` yields: `Container` itself, or
/// the container rebound to the viewed structure for a container of StructView.
template <typename Container>
using ContainerRowExtractionT = typename ContainerRowExtraction<Container>::type;

}  // namespace traits

namespace detail {

template <typename View, typename Container>
class ViewContainer {
public:
    static_assert(traits::IsStructView<View>::value, "The view of a ViewContainer must be a StructView");
    static_assert(
        std::is_same_v<typename View::UnderlyingType, std::remove_cvref_t<typename Container::value_type>>,
        "The container's elements must be the structure the view is over. "
        "Use MakeViewContainer to deduce the structure from the container"
    );

    using ViewType = View;
    using ContainerType = Container;

    explicit ViewContainer(const Container& container) : container_(&container) {}

    const Container& GetContainer() const { return *container_; }

private:
    const Container* container_;
};

}  // namespace detail

namespace traits {

template <typename T>
struct IsViewContainer : std::false_type {};

template <typename View, typename Container>
struct IsViewContainer<io::detail::ViewContainer<View, Container>> : std::true_type {};

}  // namespace traits

/// @brief A container of structures, decomposed into columns through `View`.
///
/// For ExecuteDecompose and ExecuteDecomposeBulk: each member the view names
/// becomes one array parameter, in the view's order, and the members it does
/// not name are not sent.
/// The result borrows `container`, so it must not outlive it.
template <typename View, typename Container>
detail::ViewContainer<View, Container> StructViewContainer(const Container& container) {
    return detail::ViewContainer<View, Container>{container};
}

/// @brief A StructViewContainer over `container`, deducing the viewed class
/// from its elements.
///
/// `MakeViewContainer<&User::id, &User::name>(users)` is
/// `StructViewContainer<StructView<User, &User::id, &User::name>>(users)`.
template <auto... Members, typename Container>
auto MakeViewContainer(const Container& container) {
    using ValueType = std::remove_cvref_t<typename Container::value_type>;
    return StructViewContainer<StructView<ValueType, Members...>>(container);
}

/// @brief A StructView over `obj`, deducing the viewed class from it.
///
/// Spares the caller naming the class twice: `MakeStructView<&User::id,
/// &User::name>(user)` is `StructView<User, &User::id, &User::name>{user}`.
/// The view borrows `obj`, so it must not outlive it.
template <auto... Members>
auto MakeStructView(auto&& obj) {
    return StructView<std::remove_cvref_t<decltype(obj)>, Members...>{obj};
}

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
