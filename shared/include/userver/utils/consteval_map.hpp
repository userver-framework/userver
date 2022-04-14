#pragma once

/// @file userver/utils/consteval_map.hpp
/// @brief Set, map and bimap for compile time known data sets.

#include <stdexcept>
#include <string_view>

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
constexpr void ConstevalSlowSort(Data (&data)[N], Projection proj) {
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
constexpr void ConstevalSlowSort(KeyAndValue<Key, Value> (&map)[N]) {
  impl::ConstevalSlowSort(
      map, [](const auto& v) -> decltype(auto) { return v.key; });
}

template <class Key, std::size_t N>
constexpr void ConstevalSlowSort(Key (&keys)[N]) {
  impl::ConstevalSlowSort(keys,
                          [](const auto& v) -> decltype(auto) { return v; });
}

template <class Key, std::size_t N>
class ConsinitFlatSetLinear {
 public:
  constexpr ConsinitFlatSetLinear() {}

  constexpr /*consteval*/ void Init(Key (&keys)[N]) {
    for (std::size_t i = 0; i < N; ++i) {
      keys_[i] = keys[i];
    }
  }

  constexpr std::size_t FindIndex(const Key& key) const {
    for (std::size_t i = 0; i < N; ++i) {
      if (keys_[i] == key) return i;
    }

    return N;
  }

  constexpr bool Contains(const Key& key) const { return FindIndex(key) != N; }

  constexpr const Key& AtIndex(std::size_t pos) const { return keys_[pos]; }

 private:
  Key keys_[N] = {};
};

template <class Key, class Value, std::size_t N>
class ConsinitFlatMapLinear {
 public:
  constexpr ConsinitFlatMapLinear() {}

  constexpr void Init(KeyAndValue<Key, Value> (&map)[N]) {
    Key keys[N] = {};
    Value values[N] = {};
    for (std::size_t i = 0; i < N; ++i) {
      keys[i] = map[i].key;
      values[i] = map[i].value;
    }
    keys_.Init(keys);
    values_.Init(values);
  }

  constexpr bool Contains(const Key& key) const {
    return keys_.FindIndex(key) != N;
  }

  constexpr const Value* FindOrNullptr(const Key& key) const {
    auto index = keys_.FindIndex(key);
    if (index == N) {
      return nullptr;
    }

    return &values_.AtIndex(index);
  }

 private:
  ConsinitFlatSetLinear<Key, N> keys_ = {};
  ConsinitFlatSetLinear<Value, N> values_ = {};
};

template <class Key, class Value, std::size_t N>
class ConsinitFlatBimapLinear : ConsinitFlatMapLinear<Key, Value, N> {
 public:
  using ConsinitFlatMapLinear<Key, Value, N>::ConsinitFlatMapLinear;

  constexpr bool ContainsValue(const Value& value) const {
    return values_.FindIndex(value) != N;
  }

  constexpr const Key* FindByValueOrNullptr(const Value& value) const {
    auto index = values_.FindIndex(value);
    if (index == N) {
      return nullptr;
    }

    return &keys_.AtIndex(index);
  }

 private:
  ConsinitFlatSetLinear<Key, N> keys_ = {};
  ConsinitFlatSetLinear<Value, N> values_ = {};
};

/////

template <class Key, std::size_t N>
class ConsinitFlatSetBinsearch {
 public:
  constexpr ConsinitFlatSetBinsearch() {}

  constexpr /*consteval*/ void Init(Key (&keys)[N]) {
    // Sanity checks
    for (std::size_t i = 1; i < N; ++i) {
      if (keys[i - 1] >= keys[i]) {
        throw 42;
      };
    }

    for (std::size_t i = 0; i < N; ++i) {
      keys_[i] = keys[i];
    }
  }

  constexpr std::size_t FindIndex(const Key& key) const {
    auto* begin = &keys_[0];
    auto* end = begin + N;
    do {
      auto it = begin + (end - begin) / 2;
      if (*it < key) {
        begin = it + 1;
      } else if (*it == key) {
        return it - &keys_[0];
      } else {
        end = it;
      }
    } while (end != begin);

    return N;
  }

  constexpr bool Contains(const Key& key) const { return FindIndex(key) != N; }

  constexpr const Key& AtIndex(std::size_t pos) const { return keys_[pos]; }

 private:
  template <class Value>
  static constexpr void PrepareMap(KeyAndValue<Key, Value> (&map)[N]) {
    impl::ConstevalSlowSort(map);
  }

  Key keys_[N] = {};
};

template <class Key, class Value, std::size_t N>
class ConsinitFlatMapBinsearch {
 public:
  constexpr ConsinitFlatMapBinsearch() {}

  constexpr /*consteval*/ void Init(KeyAndValue<Key, Value> (&map)[N]) {
    // Sanity checks
    for (std::size_t i = 1; i < N; ++i) {
      if (map[i - 1].key >= map[i].key) {
        throw 42;
      };
    }

    Key keys[N];
    for (std::size_t i = 0; i < N; ++i) {
      keys[i] = map[i].key;
      values_[i] = map[i].value;
    }
    keys_.Init(keys);
  }

  constexpr bool Contains(const Key& key) const {
    return keys_.FindIndex(key) != N;
  }

  constexpr const Value* FindOrNullptr(const Key& key) const {
    auto index = keys_.FindIndex(key);
    if (index == N) {
      return nullptr;
    }

    return values_ + index;
  }

 private:
  ConsinitFlatSetBinsearch<Key, N> keys_ = {};
  Value values_[N] = {};
};

}  // namespace impl

template <class Key, std::size_t N>
constexpr /*consteval*/ auto MakeConsinitSet(Key(&&keys)[N]) {
  if constexpr (N > impl::kMaxLinearSearch) {
    impl::ConstevalSlowSort(keys);
    impl::ConsinitFlatSetBinsearch<Key, N> result;
    result.Init(keys);
    return result;
  } else {
    impl::ConsinitFlatSetLinear<Key, N> result;
    result.Init(keys);
    return result;
  }
}

template <class Key, class Value, std::size_t N>
constexpr /*consteval*/ auto MakeConsinitMap(
    impl::KeyAndValue<Key, Value>(&&map)[N]) {
  if constexpr (N > impl::kMaxLinearSearch) {
    impl::ConstevalSlowSort(map);
    impl::ConsinitFlatMapBinsearch<Key, Value, N> result;
    result.Init(map);
    return result;
  } else {
    impl::ConsinitFlatMapLinear<Key, Value, N> result;
    result.Init(map);
    return result;
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
