#pragma once

/// @file userver/utils/enumerate.hpp
/// @brief @copybrief utils::enumerate
/// @ingroup userver_universal

#include <cstdint>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

template <typename Iter>
struct IteratorWrapper {
  Iter iterator;
  std::size_t pos{0};

  constexpr IteratorWrapper& operator++() {
    ++pos;
    ++iterator;
    return *this;
  }

  constexpr std::pair<const std::size_t, decltype(*iterator)> operator*()
      const {
    return {pos, *iterator};
  }

  constexpr bool operator==(const IteratorWrapper& other) const {
    return iterator == other.iterator;
  }

  constexpr bool operator!=(const IteratorWrapper& other) const {
    return !(iterator == other.iterator);
  }
};

template <typename Container,
          typename Iter = decltype(std::begin(std::declval<Container>())),
          typename = decltype(std::end(std::declval<Container>()))>
struct ContainerWrapper {
  using Iterator = IteratorWrapper<Iter>;
  Container container;

  constexpr Iterator begin() { return {std::begin(container), 0}; }
  constexpr Iterator end() { return {std::end(container), 0}; }
  constexpr Iterator begin() const { return {std::begin(container), 0}; }
  constexpr Iterator end() const { return {std::end(container), 0}; }
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
