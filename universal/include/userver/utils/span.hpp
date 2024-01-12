#pragma once

/// @file userver/utils/span.hpp
/// @brief @copybrief utils::span

#include <iterator>

#if __has_include(<span>)
#include <span>
#endif

#if __cpp_lib_span >= 202002L
#include <boost/version.hpp>
#else
#include <cstddef>
#include <type_traits>

#include <userver/utils/assert.hpp>
#endif  // __cpp_lib_span >= 202002L

USERVER_NAMESPACE_BEGIN

namespace utils {

#if __cpp_lib_span >= 202002L || defined(DOXYGEN)

/// A polyfill for std::span from C++20
using std::span;

/// A polyfill for std::as_bytes from C++20
using std::as_bytes;

/// A polyfill for std::as_writable_bytes from C++20
using std::as_writable_bytes;

#else

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

  template <
      typename Container,
      typename = std::enable_if_t<
          // Container is a range
          // Either Container is lvalue, or elements are const
          (std::is_lvalue_reference_v<Container> || std::is_const_v<T>)&&
          // Copy and move constructor fix
          !std::is_same_v<std::remove_cv_t<std::remove_reference_t<Container>>,
                          span>>>
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

  constexpr span<T> subspan(std::size_t offset, std::size_t count) const
      noexcept {
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

#endif

}  // namespace utils

USERVER_NAMESPACE_END

/// @cond

// Boost requires ranges to have a nested constant_iterator alias,
// but utils::span does not have one.
namespace boost {

template <typename T, typename Enabler>
struct range_const_iterator;

#if __cpp_lib_span >= 202002L

template <typename T, std::size_t Extent>
struct range_const_iterator<std::span<T, Extent>, void> {
  using type = typename std::span<T, Extent>::iterator;
};

#else

template <typename T>
struct range_const_iterator<USERVER_NAMESPACE::utils::span<T>, void> {
  using type = typename USERVER_NAMESPACE::utils::span<T>::iterator;
};

#endif

}  // namespace boost

// Boost < 1.77 does not mark its iterators as contiguous,
// which makes it impossible to convert Boost containers to std::span.
// Specialize std::iterator_traits as a workaround.
#if __cpp_lib_span >= 202002L && BOOST_VERSION < 107700

namespace boost::container {

template <class Pointer, bool IsConst>
class vec_iterator;

}  // namespace boost::container

template <class Pointer, bool IsConst>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::iterator_traits<boost::container::vec_iterator<Pointer, IsConst>> {
  using iterator_concept = std::contiguous_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type =
      typename boost::container::vec_iterator<Pointer, IsConst>::value_type;
  using difference_type =
      typename boost::container::vec_iterator<Pointer,
                                              IsConst>::difference_type;
  using pointer =
      typename boost::container::vec_iterator<Pointer, IsConst>::pointer;
  using reference =
      typename boost::container::vec_iterator<Pointer, IsConst>::reference;
};

#endif

/// @endcond
