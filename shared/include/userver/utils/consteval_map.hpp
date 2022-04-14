#pragma once

/// @file userver/utils/consteval_map.hpp
/// @brief Set and map for compile time known data sets.

#include <cstddef>
#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {
static constexpr std::size_t kMaxLinearSearch = 16;

template <class Key, class Value>
struct KeyAndValue {
  // Explicitly making constrcutors constexpr
  constexpr KeyAndValue() {}
  constexpr KeyAndValue(const Key& key, const Value& value)
      : key(key), value(value) {}

  Key key;
  Value value;
};

template <class Data, std::size_t N, class Projection>
constexpr void CompileTimeSlowSort(Data (&data)[N], Projection proj) noexcept {
  for (std::size_t i = 0; i < N; ++i) {
    for (std::size_t j = i + 1; j < N; ++j) {
      if (proj(data[i]) > proj(data[j])) {
        auto tmp = data[i];
        data[i] = data[j];
        data[j] = tmp;
      }
    }
  }
}

template <class Key, class Value, std::size_t N>
constexpr void CompileTimeSlowSort(KeyAndValue<Key, Value> (&map)[N]) noexcept {
  impl::CompileTimeSlowSort(
      map, [](const auto& v) -> decltype(auto) { return v.key; });
}

template <class Key, std::size_t N>
constexpr void CompileTimeSlowSort(Key (&keys)[N]) noexcept {
  impl::CompileTimeSlowSort(keys,
                            [](const auto& v) -> decltype(auto) { return v; });
}

template <class Key, std::size_t N>
constexpr std::size_t CompileTimeBinsearchFind(const Key (&keys)[N],
                                               const Key& key) noexcept {
  std::size_t begin_index = 0;
  std::size_t end_index = N;
  std::size_t distance = N;

  do {
    const auto ind = begin_index + distance / 2;
    if (keys[ind] < key) {
      begin_index = ind + 1;
    } else {
      end_index = ind + 1;
    }

    distance = end_index - begin_index;
  } while (distance > kMaxLinearSearch);

  for (; begin_index != end_index; ++begin_index) {
    if (keys[begin_index] == key) return begin_index;
  }

  return N;
}

template <class Key, std::size_t N>
constexpr std::size_t CompileTimeLinearFind(const Key (&keys)[N],
                                            const Key& key) noexcept {
  for (std::size_t i = 0; i < N; ++i) {
    if (keys[i] == key) return i;
  }

  return N;
}

template <class Key, std::size_t N>
constexpr std::size_t CompileTimeFind(const Key (&keys)[N],
                                      const Key& key) noexcept {
  if constexpr (N > impl::kMaxLinearSearch) {
    return impl::CompileTimeBinsearchFind(keys, key);
  } else {
    return impl::CompileTimeLinearFind(keys, key);
  }
}

template <class Key, std::size_t N>
constexpr void CompileTimeAssertUnique(const Key (&keys)[N]) {
  for (std::size_t i = 1; i < N; ++i) {
    if (keys[i - 1] >= keys[i]) throw std::logic_error("Duplicate values");
  }
}

}  // namespace impl

template <class Key, std::size_t N>
class ConsinitSet {
 public:
  constexpr explicit ConsinitSet(Key(&&keys)[N]) {
    impl::CompileTimeSlowSort(keys);

    for (std::size_t i = 0; i < N; ++i) {
      keys_[i] = keys[i];
    }

    impl::CompileTimeAssertUnique(keys_);
  }

  constexpr bool Contains(const Key& key) const noexcept {
    return impl::CompileTimeFind(keys_, key) != N;
  }

  const Key* begin() const noexcept { return &keys_[0]; }
  const Key* end() const noexcept { return keys_ + N; }

 private:
  Key keys_[N] = {};
};

template <class Key, std::size_t N>
constexpr /*consteval*/ ConsinitSet<Key, N> MakeConsinitSet(Key(&&keys)[N]) {
  return ConsinitSet<Key, N>(std::move(keys));
}

template <class Key, class Value, std::size_t N>
class ConsinitMap {
 public:
  constexpr explicit ConsinitMap(impl::KeyAndValue<Key, Value>(&&map)[N]) {
    impl::CompileTimeSlowSort(map);

    for (std::size_t i = 0; i < N; ++i) {
      keys_[i] = map[i].key;
      values_[i] = map[i].value;
    }

    impl::CompileTimeAssertUnique(keys_);
  }

  constexpr bool Contains(const Key& key) const noexcept {
    return impl::CompileTimeFind(keys_, key) != N;
  }

  constexpr const Value* FindOrNullptr(const Key& key) const noexcept {
    auto index = impl::CompileTimeFind(keys_, key);
    if (index == N) {
      return nullptr;
    }

    return values_ + index;
  }

 private:
  Key keys_[N] = {};
  Value values_[N] = {};
};

template <class Key, class Value, std::size_t N>
constexpr /*consteval*/ ConsinitMap<Key, Value, N> MakeConsinitMap(
    impl::KeyAndValue<Key, Value>(&&map)[N]) {
  return ConsinitMap<Key, Value, N>(std::move(map));
}

}  // namespace utils

USERVER_NAMESPACE_END
