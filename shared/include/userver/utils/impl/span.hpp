#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

template <typename T>
class Span final {
 public:
  using iterator = T*;
  using const_iterator = T*;  // Span has pointer semantics

  constexpr Span() : Span(nullptr, nullptr) {}

  constexpr Span(T* begin, T* end) noexcept : begin_(begin), end_(end) {
    UASSERT((begin != nullptr && end != nullptr && begin <= end) ||
            (begin == nullptr && end == nullptr));
  }

  template <
      typename Container,
      typename = std::enable_if_t<
          // Container is a range
          !std::is_same_v<decltype(*std::begin(std::declval<Container&&>())),
                          void> &&
          // Copy and move constructor fix
          !std::is_same_v<std::remove_cv_t<std::remove_reference_t<Container>>,
                          Span>>>
  constexpr /*implicit*/ Span(Container&& cont) noexcept
      : Span(std::data(cont), std::data(cont) + std::size(cont)) {}

  constexpr T* begin() const noexcept { return begin_; }
  constexpr T* end() const noexcept { return end_; }

  constexpr T* data() const noexcept { return begin_; }
  constexpr std::size_t size() const noexcept { return end_ - begin_; }

  constexpr T& operator[](std::size_t index) const noexcept {
    UASSERT(index < size());
    return begin_[index];
  }

 private:
  T* begin_;
  T* end_;
};

template <typename Container>
Span(Container&& cont)
    -> Span<std::remove_reference_t<decltype(*std::begin(cont))>>;

}  // namespace utils::impl

USERVER_NAMESPACE_END
