#pragma once

/// @file userver/utils/enumerate.hpp
/// @brief @copybrief utils::enumerate
/// @ingroup userver_universal

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

template <typename Iter>
auto DetectEnumerateValueType() -> std::pair<const std::size_t, decltype(*std::declval<Iter>())>;

template <typename Iter, typename... Args>
auto DetectEnumerateValueType(Args&&...) -> void;

template <typename Iter>
struct IteratorWrapper {
    using difference_type = std::ptrdiff_t;
    using value_type = decltype(DetectEnumerateValueType<Iter>());
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;

    Iter iterator;
    std::size_t pos{0};

    constexpr IteratorWrapper& operator++() {
        ++pos;
        ++iterator;
        return *this;
    }

    constexpr IteratorWrapper operator++(int) {
        IteratorWrapper copy{*this};
        ++*this;
        return copy;
    }

    constexpr value_type operator*() const { return {pos, *iterator}; }

    template <typename OtherIter>
    constexpr bool operator==(const IteratorWrapper<OtherIter>& other) const {
        return iterator == other.iterator;
    }
};

template <typename Iter>
constexpr IteratorWrapper<Iter> MakeIteratorWrapper(Iter iterator) {
    return IteratorWrapper<Iter>{.iterator = std::move(iterator), .pos = 0};
}

template <typename Container>
struct ContainerWrapper {
    constexpr auto begin() { return MakeIteratorWrapper(std::begin(container)); }

    constexpr auto end() { return MakeIteratorWrapper(std::end(container)); }

    constexpr auto begin() const { return MakeIteratorWrapper(std::begin(std::as_const(container))); }

    constexpr auto end() const { return MakeIteratorWrapper(std::end(std::as_const(container))); }

    Container container;
};

}  // namespace utils::impl

namespace utils {

/// @brief Implementation of python-style enumerate function for range-for loops
/// @param iterable: Container to iterate
/// @returns ContainerWrapper, which iterator after dereference returns pair
/// of index and reference to element. The reference is const-qualified if either
/// the wrapper itself or the underlying container is const; otherwise, it is non-const.
/// It can be used in "range based for loop" with "structured binding" like this
/// @code
/// for (auto [pos, elem] : enumerate(someContainer)) {...}
/// @endcode
template <typename Container>
constexpr auto enumerate(Container&& iterable) {  // NOLINT(readability-identifier-naming)
    return impl::ContainerWrapper<Container>{std::forward<Container>(iterable)};
}

}  // namespace utils

USERVER_NAMESPACE_END
