#pragma once

/// @file userver/utils/algo.hpp
/// @brief Small useful algorithms.
/// @ingroup userver_universal

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <userver/utils/checked_pointer.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief General-purpose utilities used across userver libraries.
namespace utils {

/// @brief Concatenates multiple `std::string_view`-convertible items
template <typename ResultString = std::string, typename... Strings>
ResultString StrCat(const Strings&... strings) {
    return [](auto... string_views) {
        std::size_t result_size = 0;
        ((result_size += string_views.size()), ...);

        ResultString result;
        result.reserve(result_size);
        (result.append(string_views), ...);
        return result;
    }(std::string_view{strings}...);
}

namespace impl {

template <typename Container>
concept HasMappedType = requires { typename Container::mapped_type; };

}  // namespace impl

/// @brief Returns nullptr if no key in associative container, otherwise
/// returns pointer to value.
template <typename Container, typename Key>
auto* FindOrNullptr(Container& container, const Key& key) {
    const auto it = container.find(key);
    if constexpr (impl::HasMappedType<Container>) {
        return (it != std::ranges::end(container) ? std::addressof(it->second) : nullptr);
    } else {
        return (it != std::ranges::end(container) ? std::addressof(*it) : nullptr);
    }
}

/// @brief Returns default value if no key in associative container, otherwise
/// returns a copy of the stored value.
template <typename Container, typename Key, typename Default>
auto FindOrDefault(Container& container, const Key& key, Default&& def) {
    const auto* ptr = USERVER_NAMESPACE::utils::FindOrNullptr(container, key);
    return (ptr ? *ptr : decltype(*ptr){std::forward<Default>(def)});
}

/// @brief Returns default value if no key in associative container, otherwise
/// returns a copy of the stored value.
template <typename Container, typename Key>
auto FindOrDefault(Container& container, const Key& key) {
    const auto* ptr = USERVER_NAMESPACE::utils::FindOrNullptr(container, key);
    return (ptr ? *ptr : decltype(*ptr){});
}

/// @brief Returns std::nullopt if no key in associative container, otherwise
/// returns std::optional with a copy of value
template <typename Container, typename Key>
auto FindOptional(Container& container, const Key& key) {
    const auto* ptr = USERVER_NAMESPACE::utils::FindOrNullptr(container, key);
    return (ptr ? std::make_optional(*ptr) : std::nullopt);
}

/// @brief Searches a map for an element and return a checked pointer to
/// the found element
template <typename Container, typename Key>
auto CheckedFind(Container& container, const Key& key) {
    return utils::MakeCheckedPtr(USERVER_NAMESPACE::utils::FindOrNullptr(container, key));
}

/// @brief Converts one container type to another
///
/// @warning This function moves from elements if the range is an rvalue. This is not correct for rvalue ranges that
/// do not own their elements, such as typical views. For example:
/// @code
/// std::vector<std::string> v = {"hello", "world"};
/// auto result = utils::AsContainer<std::vector<std::string>>(
///     v | std::views::filter([](const auto& item) { return true; })
/// );
/// @endcode
/// This will move the items from `v` to `result`, which is not what you want if you want to keep the original vector.
///
/// Use C++23 `std::ranges::to` instead if possible, optionally paired with `std::ranges::as_rvalue`.
template <typename ToContainer, typename FromContainer>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
ToContainer AsContainer(FromContainer&& container) {
    if constexpr (std::is_rvalue_reference_v<decltype(container)>) {
        return ToContainer(
            std::make_move_iterator(std::ranges::begin(container)),
            std::make_move_iterator(std::ranges::end(container))
        );
    } else {
        return ToContainer(std::ranges::begin(container), std::ranges::end(container));
    }
}

namespace impl {

template <typename ToContainer, typename Range>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
ToContainer AsContainerViaInsert(Range&& range) {
    ToContainer result;
    if constexpr (requires { result.reserve(std::ranges::size(range)); }) {
        result.reserve(std::ranges::size(range));
    }
    for (auto&& ref : range) {
        result.insert(std::ranges::end(result), std::forward<decltype(ref)>(ref));
    }
    return result;
}

template <typename Container>
concept HasKeyType = requires { typename Container::key_type; };

}  // namespace impl

/// @brief Erased elements and returns number of deleted elements
template <typename Container, typename Pred>
std::integral auto EraseIf(Container& container, Pred pred) {
    if constexpr (impl::HasKeyType<Container>) {
        auto old_size = std::ranges::size(container);
        for (auto it = std::ranges::begin(container), last = std::ranges::end(container); it != last;) {
            if (pred(*it)) {
                it = container.erase(it);
            } else {
                ++it;
            }
        }
        return old_size - std::ranges::size(container);
    } else {
        auto garbage = std::ranges::remove_if(container, pred);
        container.erase(garbage.begin(), garbage.end());
        return std::ranges::size(garbage);
    }
}

/// @brief Erased elements and returns number of deleted elements
template <typename Container, typename T>
std::integral auto Erase(Container& container, const T& elem) {
    if constexpr (impl::HasKeyType<Container>) {
        return container.erase(elem);
    } else {
        // NOLINTNEXTLINE(readability-qualified-auto)
        auto garbage = std::ranges::remove(container, elem);
        container.erase(garbage.begin(), garbage.end());
        return std::ranges::size(garbage);
    }
}

/// @brief returns true if there is an element in the container which satisfies the predicate.
///
/// @deprecated Use `std::ranges::any_of` instead.
template <typename Container, typename Pred>
bool ContainsIf(const Container& container, Pred pred) {
    return std::ranges::any_of(container, pred);
}

/// @brief returns true if there is a specified element in the container
///
/// In C++23, use `std::ranges::contains` instead.
template <typename Container, typename Item>
bool Contains(const Container& container, const Item& item) {
    return std::ranges::find(container, item) != std::ranges::end(container);
}

}  // namespace utils

USERVER_NAMESPACE_END
