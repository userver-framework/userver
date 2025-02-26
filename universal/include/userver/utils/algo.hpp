#pragma once

/// @file userver/utils/algo.hpp
/// @brief Small useful algorithms.
/// @ingroup userver_universal

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <userver/utils/checked_pointer.hpp>

USERVER_NAMESPACE_BEGIN

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
template <typename Container, typename = void>
struct HasMappedType : std::false_type {};

template <typename Container>
struct HasMappedType<Container, std::void_t<typename Container::mapped_type>> : std::true_type {};

template <typename Container>
inline constexpr bool kHasMappedType = HasMappedType<Container>::value;
}  // namespace impl

/// @brief Returns nullptr if no key in associative container, otherwise
/// returns pointer to value.
template <typename Container, typename Key>
auto* FindOrNullptr(Container& container, const Key& key) {
    const auto it = container.find(key);
    if constexpr (impl::kHasMappedType<Container>) {
        return (it != container.end() ? &(it->second) : nullptr);
    } else {
        return (it != container.end() ? &(*it) : nullptr);
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
template <typename ToContainer, typename FromContainer>
ToContainer AsContainer(FromContainer&& container) {
    if constexpr (std::is_rvalue_reference_v<decltype(container)>) {
        return ToContainer(
            std::make_move_iterator(std::begin(container)), std::make_move_iterator(std::end(container))
        );
    } else {
        return ToContainer(std::begin(container), std::end(container));
    }
}

namespace impl {
template <typename Container, typename = void>
struct HasKeyType : std::false_type {};

template <typename Container>
struct HasKeyType<Container, std::void_t<typename Container::key_type>> : std::true_type {};

template <typename Container>
inline constexpr bool kHasKeyType = HasKeyType<Container>::value;
}  // namespace impl

/// @brief Erased elements and returns number of deleted elements
template <typename Container, typename Pred>
auto EraseIf(Container& container, Pred pred) {
    if constexpr (impl::kHasKeyType<Container>) {
        auto old_size = container.size();
        for (auto it = std::begin(container), last = std::end(container); it != last;) {
            if (pred(*it)) {
                it = container.erase(it);
            } else {
                ++it;
            }
        }
        return old_size - container.size();
    } else {
        auto it = std::remove_if(std::begin(container), std::end(container), pred);
        const auto removed = std::distance(it, std::end(container));
        container.erase(it, std::end(container));
        return removed;
    }
}

/// @brief Erased elements and returns number of deleted elements
template <typename Container, typename T>
size_t Erase(Container& container, const T& elem) {
    if constexpr (impl::kHasKeyType<Container>) {
        return container.erase(elem);
    } else {
        // NOLINTNEXTLINE(readability-qualified-auto)
        auto it = std::remove(std::begin(container), std::end(container), elem);
        const auto removed = std::distance(it, std::end(container));
        container.erase(it, std::end(container));
        return removed;
    }
}

/// @brief returns true if there is an element in container which satisfies
/// the predicate
template <typename Container, typename Pred>
bool ContainsIf(const Container& container, Pred pred) {
    return std::find_if(std::begin(container), std::end(container), pred) != std::end(container);
}

}  // namespace utils

USERVER_NAMESPACE_END
