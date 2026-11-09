#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <iterator>
#include <memory>
#include <ranges>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <userver/utils/boost_uuid4.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

namespace impl {

template <typename Item>
std::ranges::iterator_t<const std::set<Item>> FindDuplicateItem(const std::set<Item>& items) {
    return items.end();
}

template <typename Item>
std::ranges::iterator_t<const std::unordered_set<Item>> FindDuplicateItem(const std::unordered_set<Item>& items) {
    return items.end();
}

template <typename Container>
std::ranges::iterator_t<const Container> FindDuplicateItem(const Container& items) {
    using Item = typename Container::value_type;
    static_assert(std::totally_ordered<Item>, "uniqueItems requires an item type satisfying std::totally_ordered");

    if (items.size() < 2) {
        return items.end();
    }

    if constexpr (std::is_same_v<Item, bool>) {
        const auto first = items.begin();
        const auto second = std::next(first);
        if (*first == *second) {
            return second;
        }
        if (items.size() > 2) {
            return std::next(second);
        }
    } else if constexpr (std::is_trivially_copyable_v<Item> && sizeof(Item) <= 2 * sizeof(const Item*)) {
        std::vector<Item> sorted(items.begin(), items.end());
        if constexpr (std::is_floating_point_v<Item>) {
            std::erase_if(sorted, [](Item item) { return std::isnan(item); });
        }
        std::ranges::sort(sorted);
        const auto duplicate = std::ranges::adjacent_find(sorted);
        if (duplicate != sorted.end()) {
            return std::ranges::find(items, *duplicate);
        }
    } else {
        std::vector<const Item*> sorted;
        sorted.reserve(items.size());
        for (const auto& item : items) {
            sorted.push_back(std::addressof(item));
        }
        const auto dereference = [](const Item* item) -> const Item& { return *item; };
        std::ranges::sort(sorted, {}, dereference);
        const auto duplicate = std::ranges::adjacent_find(sorted, {}, dereference);
        if (duplicate != sorted.end()) {
            return std::ranges::find(items, *duplicate, [](const Item& item) { return std::addressof(item); });
        }
    }
    return items.end();
}

}  // namespace impl

struct UniqueItems final {
    template <typename Container, typename ErrorReporter>
    static void Validate(const Container& value, ErrorReporter report_error) {
        if (const auto duplicate = impl::FindDuplicateItem(value); duplicate != value.end()) {
            using Item = typename Container::value_type;
            if constexpr (fmt::is_formattable<Item>::value &&
                          (!fmt::is_range<Item, char>::value || std::is_same_v<Item, boost::uuids::uuid>))
            {
                const Item& item = *duplicate;
                report_error(fmt::format("Duplicate items are not allowed ({})", item));
            } else {
                report_error("Duplicate items are not allowed");
            }
        }
    }
};

}  // namespace chaotic

USERVER_NAMESPACE_END
