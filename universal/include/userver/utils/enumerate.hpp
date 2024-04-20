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
auto DetectEnumerateValueType()
    -> std::pair<const std::size_t, decltype(*std::declval<Iter>())>;

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

  constexpr value_type operator*() const { return {pos, *iterator}; }

  template <typename OtherIter>
  constexpr bool operator==(const IteratorWrapper<OtherIter>& other) const {
    return iterator == other.iterator;
  }

  template <typename OtherIter>
  constexpr bool operator!=(const IteratorWrapper<OtherIter>& other) const {
    return !(iterator == other.iterator);
  }
};

template <typename Range>
using IteratorTypeOf = decltype(std::begin(std::declval<Range&>()));

template <typename Range>
using SentinelTypeOf = decltype(std::end(std::declval<Range&>()));

template <typename Container>
struct ContainerWrapper {
  constexpr IteratorWrapper<IteratorTypeOf<Container>> begin() {
    return {std::begin(container), 0};
  }

  constexpr IteratorWrapper<SentinelTypeOf<Container>> end() {
    return {std::end(container), 0};
  }

  constexpr IteratorWrapper<IteratorTypeOf<const Container>> begin() const {
    return {std::begin(container), 0};
  }

  constexpr IteratorWrapper<SentinelTypeOf<const Container>> end() const {
    return {std::end(container), 0};
  }

  Container container;
};

}  // namespace utils::impl

namespace utils {

/// @brief Implementation of python-style enumerate function for range-for loops
/// @param iterable: Container to iterate
/// @returns ContainerWrapper, which iterator after dereference returns pair
/// of index and (!!!)non-const reference to element(it seems impossible to make
/// this reference const). It can be used in "range based for loop" with
/// "structured binding" like this
/// @code
/// for (auto [pos, elem] : enumerate(someContainer)) {...}
/// @endcode
template <typename Container>
constexpr auto enumerate(Container&& iterable) {
  return impl::ContainerWrapper<Container>{std::forward<Container>(iterable)};
}

}  // namespace utils

USERVER_NAMESPACE_END
