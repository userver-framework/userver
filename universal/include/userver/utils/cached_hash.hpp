#pragma once

/// @file userver/utils/cached_hash.hpp
/// @brief @copybrief utils::CachedHash

#include <functional>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Holds the key and its hash for faster comparisons and hashing
template <class Key>
struct CachedHash final {
  std::size_t hash;
  Key key;
};

/// @brief Compares utils::CachedHash by hash first and then by keys
template <class T>
constexpr bool operator==(const CachedHash<T>& x, const CachedHash<T>& y) {
  return x.hash == y.hash && x.key == y.key;
}

/// @brief Compares utils::CachedHash by hash first and then by keys
template <class T>
constexpr bool operator!=(const CachedHash<T>& x, const CachedHash<T>& y) {
  return !(x.key == y.key);
}

/// @brief Compares utils::CachedHash only by keys
template <class Equal, class = std::enable_if_t<!std::is_final_v<Equal>>>
class CachedHashKeyEqual : private Equal {
 public:
  explicit constexpr CachedHashKeyEqual(const Equal& eq) : Equal(eq) {}

  template <class T>
  constexpr bool operator()(const CachedHash<T>& x,
                            const CachedHash<T>& y) const {
    return Equal::operator()(x.key, y.key);
  }
};

template <class Equal>
class CachedHashKeyEqual<Equal, std::false_type> {
 public:
  explicit constexpr CachedHashKeyEqual(const Equal& eq) : equality_(eq) {}

  template <class T>
  constexpr bool operator()(const CachedHash<T>& x,
                            const CachedHash<T>& y) const {
    return equality_(x.key, y.key);
  }

 private:
  Equal equality_;
};

}  // namespace utils

USERVER_NAMESPACE_END

template <class T>
struct std::hash<USERVER_NAMESPACE::utils::CachedHash<T>> {
  constexpr std::size_t operator()(
      const USERVER_NAMESPACE::utils::CachedHash<T>& value) const noexcept {
    return value.hash;
  }
};
