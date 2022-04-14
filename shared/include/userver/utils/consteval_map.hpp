#pragma once

/// @file userver/utils/consteval_map.hpp
/// @brief Set and map for compile time known data sets.

#include <cstddef>
#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {
static constexpr std::size_t kMaxLinearSearch = 8;

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
constexpr void CompileTimeSlowSort(Data (&data)[N], Projection proj) {
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
constexpr void CompileTimeSlowSort(KeyAndValue<Key, Value> (&map)[N]) {
  impl::CompileTimeSlowSort(
      map, [](const auto& v) -> decltype(auto) { return v.key; });
}

template <class Key, std::size_t N>
constexpr void CompileTimeSlowSort(Key (&keys)[N]) {
  impl::CompileTimeSlowSort(keys,
                            [](const auto& v) -> decltype(auto) { return v; });
}

template <class Key, std::size_t N>
constexpr std::size_t CompileTimeBinsearchFind(const Key (&keys)[N],
                                               const Key& key) {
  auto* begin = &keys[0];
  auto* end = begin + N;
  do {
    auto it = begin + (end - begin) / 2;
    if (*it < key) {
      begin = it + 1;
    } else if (*it == key) {
      return it - &keys[0];
    } else {
      end = it;
    }
  } while (end != begin);

  return N;
}

template <class Key, std::size_t N>
constexpr std::size_t CompileTimeLinearFind(const Key (&keys)[N],
                                            const Key& key) {
  for (std::size_t i = 0; i < N; ++i) {
    if (keys[i] == key) return i;
  }

  return N;
}

template <class Key, std::size_t N>
constexpr std::size_t CompileTimeFind(const Key (&keys)[N], const Key& key) {
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
  constexpr explicit ConsinitSet(Key (&keys)[N]) {
    impl::CompileTimeSlowSort(keys);

    for (std::size_t i = 0; i < N; ++i) {
      keys_[i] = keys[i];
    }

    impl::CompileTimeAssertUnique(keys_);
  }

  constexpr bool Contains(const Key& key) const {
    return impl::CompileTimeFind(keys_, key) != N;
  }

 private:
  Key keys_[N] = {};
};

template <class Key, std::size_t N>
constexpr /*consteval*/ ConsinitSet<Key, N> MakeConsinitSet(Key(&&keys)[N]) {
  return ConsinitSet<Key, N>(keys);
}

template <class Key, class Value, std::size_t N>
class ConsinitMap {
 public:
  constexpr explicit ConsinitMap(impl::KeyAndValue<Key, Value> (&map)[N]) {
    impl::CompileTimeSlowSort(map);

    for (std::size_t i = 0; i < N; ++i) {
      keys_[i] = map[i].key;
      values_[i] = map[i].value;
    }

    impl::CompileTimeAssertUnique(keys_);
  }

  constexpr bool Contains(const Key& key) const {
    return impl::CompileTimeFind(keys_, key) != N;
  }

  constexpr const Value* FindOrNullptr(const Key& key) const {
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
  return ConsinitMap<Key, Value, N>(map);
}

}  // namespace utils

USERVER_NAMESPACE_END
