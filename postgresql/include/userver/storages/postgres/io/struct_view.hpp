#pragma once

/// @file userver/storages/postgres/io/partial_struct.hpp
/// @brief I/O support for partial struct read
/// @ingroup userver_postgres_parse_and_format

#include <tuple>
#include <type_traits>

#include <boost/pfr.hpp>

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
    if constexpr (sizeof...(Members) > 1) {
        using OwnerType = std::remove_cvref_t<decltype(value)>;
        static_assert(traits::kAllMembersOf<OwnerType, Members...>, "All members must be members of the same class");
        return std::tie(std::forward<decltype(value)>(value).*Members...);
    } else {
        return boost::pfr::structure_tie(std::forward<decltype(value)>(value));
    }
}

template <typename Alloc, typename NewType>
struct RebindAllocator {
    using type = Alloc;  // No rebind for non-template allocators
};

template <template <typename, typename...> typename Alloc, typename OldT, typename NewT, typename... Args>
struct RebindAllocator<Alloc<OldT, Args...>, NewT> {
    using type = Alloc<NewT, Args...>;
};

template <typename Container, typename NewType>
struct RebindContainer;

template <template <typename, typename> typename Container, typename OldT, typename NewT, typename Alloc>
struct RebindContainer<Container<OldT, Alloc>, NewT> {
    using type = Container<NewT, typename RebindAllocator<Alloc, NewT>::type>;
};

template <template <typename, auto...> typename Container, typename OldT, typename NewT, auto... Args>
struct RebindContainer<Container<OldT, Args...>, NewT> {
    using type = Container<NewT, Args...>;
};

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
    requires std::is_convertible_v<U, UnderlyingType>
    static auto Tie(U&& value) {
        return detail::PartialTie<Members...>(std::forward<U>(value));
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

}  // namespace traits

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
