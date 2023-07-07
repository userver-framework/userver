#pragma once

/// @file userver/utils/enumerate.hpp
/// @brief @copybrief utils::enumerate

#include <tuple>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

template <typename Iter>
struct IteratorWrapper {
  Iter iterator;
  size_t pos{0};

  constexpr auto& operator++() {
    ++pos;
    ++iterator;
    return *this;
  }
  constexpr std::tuple<const size_t, decltype(*iterator)> operator*() const {
    return {pos, *iterator};
  }
  constexpr std::tuple<const size_t, decltype(*iterator)> operator*() {
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

  constexpr auto begin() { return Iterator{std::begin(container), 0}; }
  constexpr auto end() { return Iterator{std::end(container), 0}; }
  constexpr auto begin() const { return Iterator{std::begin(container), 0}; }
  constexpr auto end() const { return Iterator{std::end(container), 0}; }
};
}  // namespace utils::impl

namespace utils {

/// @brief Implementation of python-style enumerate function for range-for loops
/// @param iterable: Container to iterate
/// @returns ContainerWrapper, which iterator after dereference returns tuple
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
