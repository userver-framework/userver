#pragma once

/// @file userver/utils/meta.hpp
/// @brief Metaprogramming, template variables and concepts
/// @ingroup userver_universal

#include <concepts>
#include <iosfwd>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

#include <userver/compiler/impl/nodebug.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Metaprogramming utilities, concepts, and type traits helpers.
namespace meta {

namespace impl {

using std::begin;
using std::end;

template <typename T>
concept IsRange = requires(T& t) {
    {
        begin(t)
    };
    {
        end(t)
    };
};

template <IsRange T>
using IteratorType USERVER_IMPL_NODEBUG = decltype(begin(std::declval<T&>()));

template <typename T>
using RangeValueType USERVER_IMPL_NODEBUG = typename std::iterator_traits<IteratorType<T>>::value_type;

template <typename T>
struct IsFixedSizeContainer : std::false_type {};

// Boost and std arrays
template <typename T, std::size_t Size, template <typename, std::size_t> typename Array>
struct IsFixedSizeContainer<Array<T, Size>> : std::bool_constant<sizeof(Array<T, Size>) == sizeof(T) * Size> {};

template <typename... Args>
concept IsSingleRange = (sizeof...(Args) == 1) && (impl::IsRange<Args> && ...);

}  // namespace impl

/// @warning Use std::ranges::range instead of this concept, except possibly in common headers
/// where compilation time is a concern.
template <typename T>
concept IsRange = impl::IsRange<T>;

/// Returns true if T is an ordered or unordered map or multimap
template <typename T>
concept IsMap = IsRange<T> && requires {
    typename T::key_type;
    typename T::mapped_type;
};

/// Returns true if T is a map (but not a multimap!)
template <typename T>
concept IsUniqueMap = IsMap<T> && requires(T& map, typename T::key_type key) {
    map[key];  // no operator[] in multimaps
};

template <IsMap T>
using MapKeyType = typename T::key_type;

template <IsMap T>
using MapValueType = typename T::mapped_type;

/// @warning Use std::ranges::range_value_t instead of this type, except possibly in common headers
/// where compilation time is a concern.
template <IsRange T>
using RangeValueType = impl::RangeValueType<T>;

template <typename T>
concept IsRecursiveRange = IsRange<T> && std::same_as<impl::RangeValueType<T>, T>;

template <typename T>
concept IsOptional = kIsInstantiationOf<std::optional, T>;

template <typename T>
concept IsOstreamWritable = requires(std::ostream& os, const std::remove_reference_t<T>& val) {
    {
        os << val
    } -> std::same_as<std::ostream&>;
};

template <typename T>
concept IsStdHashable = requires(const T& val) {
    {
        std::hash<T>{}(val)
    } -> std::same_as<std::size_t>;
} && std::equality_comparable<T>;

/// @brief Check if std::size is applicable to container
/// @warning Use std::ranges::sized_range instead of this concept, except possibly in common headers
/// where compilation time is a concern.
template <typename T>
concept IsSizable = IsRange<T> && requires(T value) { std::size(value); };

/// @brief Check if a container has `reserve`
template <typename T>
concept IsReservable = IsSizable<T> && requires(T value) { value.reserve(1); };

/// @brief Check if a container has 'push_back'
template <typename T>
concept IsPushBackable = IsRange<T> && requires(T value, RangeValueType<T> element) {
    value.push_back(std::move(element));
};

/// @brief Check if a container has fixed size (e.g. `std::array`)
template <typename T>
concept IsFixedSizeContainer = IsRange<T> && impl::IsFixedSizeContainer<T>::value;

template <typename T>
concept IsVectorLike = IsRange<T> && std::default_initializable<T> && IsReservable<T> && IsPushBackable<T>;

/// @brief Returns default inserter for a container
template <typename T>
auto Inserter(T& container) {
    if constexpr (IsPushBackable<T>) {
        return std::back_inserter(container);
    } else if constexpr (IsFixedSizeContainer<T>) {
        return container.begin();
    } else {
        return std::inserter(container, container.end());
    }
}

/// @deprecated Use @ref meta::IsVectorLike instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsVector = IsVectorLike<T>;

/// @deprecated Use @ref meta::IsRange instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsRange = IsRange<T>;

/// @deprecated Use @ref meta::IsMap instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsMap = IsMap<T>;

/// @deprecated Use @ref meta::IsOptional instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsOptional = IsOptional<T>;

/// @deprecated Use @ref meta::IsSizable instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsSizable = IsSizable<T>;

/// @deprecated Use @ref meta::IsReservable instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsReservable = IsReservable<T>;

}  // namespace meta

USERVER_NAMESPACE_END
