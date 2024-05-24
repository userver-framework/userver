#pragma once

/// @file userver/utils/span.hpp
/// @brief @copybrief utils::span

#include <cstddef>
#include <iterator>
#include <type_traits>

#if __has_include(<span>)
#include <span>
#endif

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// A polyfill for std::span from C++20
template <typename T>
class span final {
 public:
  using iterator = T*;

  constexpr span() noexcept : span(nullptr, nullptr) {}

  constexpr span(T* begin, T* end) noexcept : begin_(begin), end_(end) {
    UASSERT((begin != nullptr && end != nullptr && begin <= end) ||
            (begin == nullptr && end == nullptr));
  }

  constexpr span(T* begin, std::size_t size) noexcept
      : begin_(begin), end_(begin + size) {
    UASSERT(begin != nullptr || size == 0);
  }

  template <
      typename Container,
      typename = std::enable_if_t<
          // Either Container is lvalue, or this span's elements are const
          (std::is_lvalue_reference_v<Container> || std::is_const_v<T>)&&
          // Copy and move constructor fix
          !std::is_same_v<std::remove_cv_t<std::remove_reference_t<Container>>,
                          span> &&
          // Container is a range of T
          std::is_convertible_v<decltype(std::data(std::declval<Container&>())),
                                T*>>>
  // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
  constexpr /*implicit*/ span(Container&& cont) noexcept
      : span(std::data(cont), std::data(cont) + std::size(cont)) {}

  constexpr T* begin() const noexcept { return begin_; }
  constexpr T* end() const noexcept { return end_; }

  constexpr T* data() const noexcept { return begin_; }
  constexpr std::size_t size() const noexcept { return end_ - begin_; }
  constexpr bool empty() const noexcept { return size() == 0; }

  constexpr span<T> first(std::size_t count) const noexcept {
    UASSERT(count <= size());
    return span{begin_, begin_ + count};
  }

  constexpr span<T> last(std::size_t count) const noexcept {
    UASSERT(count <= size());
    return span{end_ - count, end_};
  }

  constexpr span<T> subspan(std::size_t offset) const noexcept {
    UASSERT(offset <= size());
    return span{begin_ + offset, end_};
  }

  constexpr span<T> subspan(std::size_t offset,
                            std::size_t count) const noexcept {
    UASSERT(offset + count <= size());
    return span{begin_ + offset, begin_ + offset + count};
  }

  constexpr T& operator[](std::size_t index) const noexcept {
    UASSERT(index < size());
    return begin_[index];
  }

 private:
  T* begin_;
  T* end_;
};

template <typename Container>
span(Container&& cont)
    -> span<std::remove_reference_t<decltype(*std::begin(cont))>>;

/// A polyfill for std::as_bytes from C++20
template <typename T>
span<const std::byte> as_bytes(span<T> s) noexcept {
  const auto* const data = reinterpret_cast<const std::byte*>(s.data());
  return {data, data + s.size() * sizeof(T)};
}

/// A polyfill for std::as_writable_bytes from C++20
template <typename T>
span<std::byte> as_writable_bytes(span<T> s) noexcept {
  static_assert(!std::is_const_v<T>);
  auto* const data = reinterpret_cast<std::byte*>(s.data());
  return {data, data + s.size() * sizeof(T)};
}

}  // namespace utils

USERVER_NAMESPACE_END

/// @cond

// Boost requires ranges to have a nested constant_iterator alias,
// but utils::span does not have one.
namespace boost {

template <typename T, typename Enabler>
struct range_const_iterator;

template <typename T>
struct range_const_iterator<USERVER_NAMESPACE::utils::span<T>, void> {
  using type = typename USERVER_NAMESPACE::utils::span<T>::iterator;
};

}  // namespace boost

/// @endcond
