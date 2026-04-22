#pragma once

#include <iosfwd>
#include <tuple>
#include <type_traits>

#include <userver/utils/meta.hpp>

#include <userver/storages/postgres/detail/is_decl_complete.hpp>
#include <userver/storages/postgres/io/io_fwd.hpp>
#include <userver/storages/postgres/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io::traits {

template <bool Value>
using BoolConstant = std::bool_constant<Value>;
template <std::size_t Value>
using SizeConstant = std::integral_constant<std::size_t, Value>;

///@{
/** @name Type mapping traits */
/// @brief Detect if the C++ type is mapped to a Postgres system type.
template <typename T>
concept IsMappedToSystemType = utils::IsDeclComplete<CppToSystemPg<T>>::value;

/// @brief Detect if the C++ type is mapped to a Postgres user type.
template <typename T>
concept IsMappedToUserType = utils::IsDeclComplete<CppToUserPg<T>>::value;

namespace detail {

template <typename Container>
constexpr bool EnableContainerMapping() noexcept;

}  // namespace detail

/// @brief Detect if the C++ type is mapped to a Postgres array type.
template <typename T>
concept IsMappedToArray = detail::EnableContainerMapping<T>();

/// @brief Detect if the C++ type is mapped to a Postgres type.
template <typename T>
struct IsMappedToPg : BoolConstant<IsMappedToUserType<T> || IsMappedToSystemType<T> || IsMappedToArray<T>> {};
template <typename T>
concept kIsMappedToPg = IsMappedToPg<T>::value;  // NOLINT(readability-identifier-naming)

/// @brief Mark C++ mapping a special case for disambiguation.
template <typename T>
struct IsSpecialMapping : std::false_type {};
///@}

///@{
/** @name Traits for containers */
/// @brief Mark C++ container type as supported by the driver.
template <typename T>
struct IsCompatibleContainer : std::false_type {};
template <typename T>
concept kIsCompatibleContainer = IsCompatibleContainer<T>::value;  // NOLINT(readability-identifier-naming)
/// @}

///@{
/// @brief Calculate number of dimensions in C++ container.
template <typename T>
struct DimensionCount : SizeConstant<0> {};

template <kIsCompatibleContainer T>
struct DimensionCount<T> : SizeConstant<1 + DimensionCount<typename T::value_type>::value> {};
template <typename T>
inline constexpr std::size_t kDimensionCount = DimensionCount<T>::value;
///@}

///@{
/// @brief Detect type of multidimensional C++ container.
template <typename T>
struct ContainerFinalElement;

namespace detail {

template <typename T>
struct FinalElementImpl {
    using type = T;
};

template <typename T>
struct ContainerFinalElementImpl {
    using type = typename ContainerFinalElement<typename T::value_type>::type;
};

}  // namespace detail

template <typename T>
struct ContainerFinalElement
    : std::conditional_t<kIsCompatibleContainer<T>, detail::ContainerFinalElementImpl<T>, detail::FinalElementImpl<T>> {
};

template <typename T>
using ContainerFinaleElementType = typename ContainerFinalElement<T>::type;
///@}

namespace detail {

template <typename Container>
constexpr bool EnableContainerMapping() noexcept {
    if constexpr (!traits::kIsCompatibleContainer<Container>) {
        return false;
    } else {
        return traits::kIsMappedToPg<typename traits::ContainerFinalElement<Container>::type>;
    }
}

}  // namespace detail

template <typename T>
concept CanReserve = requires(T value) { value.reserve(std::size_t{1}); };

template <typename T>
concept CanResize = requires(T value) { value.resize(std::size_t{1}); };

template <typename T>
concept CanClear = requires(T value) { value.clear(); };

template <typename T>
auto Inserter(T& container) {
    return meta::Inserter(container);
}

template <typename T>
struct RemoveTupleReferences;

template <typename... T>
struct RemoveTupleReferences<std::tuple<T...>> {
    using type = std::tuple<std::remove_reference_t<T>...>;
};

template <typename T>
struct IsTupleOfRefs : std::false_type {};
template <typename... T>
struct IsTupleOfRefs<std::tuple<T&...>> : std::true_type {};

template <typename T>
struct AddTupleConstRef;

template <typename... T>
struct AddTupleConstRef<std::tuple<T...>> {
    using type = std::tuple<const std::remove_reference_t<T>&...>;
};

template <typename Tuple>
struct TupleHasParsers;

template <typename... T>
struct TupleHasParsers<std::tuple<T...>> : std::bool_constant<(HasParser<std::remove_cvref_t<T>> && ...)> {};

template <typename Tuple>
struct TupleHasFormatters;

template <typename... T>
struct TupleHasFormatters<std::tuple<T...>> : std::bool_constant<(HasFormatter<std::remove_cvref_t<T>> && ...)> {};

}  // namespace storages::postgres::io::traits

USERVER_NAMESPACE_END
