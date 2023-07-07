#pragma once

/// @file userver/utils/filter_bloom.hpp
/// @brief @copybrief utils::FilterBloom

#include <array>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

#include <boost/container_hash/hash.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Space-efficient probabilistic data structure
///
/// @details Used to test whether a count number of a given element is
/// smaller than a given threshold when a sequence of elements is given.
/// As a generalized form of Bloom filter,
/// false positive matches are possible, but false negatives are not.
/// @param T the type of element that counts
/// @param Counter the type of counter
/// @param Hash1 the first callable hash struct
/// @param Hash2 the second callable hash struct
///
/// Example:
/// @snippet src/utils/filter_bloom_test.cpp  Sample filter bloom usage
template <typename T, typename Counter = unsigned,
          typename Hash1 = boost::hash<T>, typename Hash2 = std::hash<T>>
class FilterBloom final {
 public:
  /// @brief Constructs filter Bloom with the specified number of counters
  /// @note If expected to increment n times is recommended to set counters_num
  /// to 16 * n
  explicit FilterBloom(std::size_t counters_num = 256, Hash1 hash_1 = Hash1{},
                       Hash2 hash_2 = Hash2{})
      : counters_(counters_num, 0),
        hasher_1(std::move(hash_1)),
        hasher_2(std::move(hash_2)) {
    UASSERT((!std::is_same_v<Hash1, Hash2>));
    UASSERT((std::is_same_v<std::invoke_result_t<Hash1, const T&>,
                            std::invoke_result_t<Hash2, const T&>>));
  }

  /// @brief Increments the smallest item counters
  void Increment(const T& item);

  /// @brief Returns the value of the smallest item counter
  Counter Estimate(const T& item) const;

  /// @brief Checks that all counters of the item have been incremented
  bool Has(const T& item) const;

  /// @brief Resets all counters
  void Clear();

 private:
  using HashedType = std::invoke_result_t<Hash1, const T&>;

  inline uint64_t Coefficient(std::size_t step) const;
  uint64_t GetHash(const HashedType& hashed_value_1,
                   const HashedType& hashed_value_2,
                   uint64_t coefficient) const;
  Counter MinFrequency(const HashedType& hashed_value_1,
                       const HashedType& hashed_value_2) const;

  utils::FixedArray<Counter> counters_;
  const Hash1 hasher_1;
  const Hash2 hasher_2;

  static constexpr std::size_t kHashFunctionsCount = 4;
};

template <typename T, typename Counter, typename Hash1, typename Hash2>
inline uint64_t FilterBloom<T, Counter, Hash1, Hash2>::Coefficient(
    std::size_t step) const {
  return 1 << step;
}

template <typename T, typename Counter, typename Hash1, typename Hash2>
uint64_t FilterBloom<T, Counter, Hash1, Hash2>::GetHash(
    const HashedType& hashed_value_1, const HashedType& hashed_value_2,
    uint64_t coefficient) const {
  // the idea was taken from
  // https://www.eecs.harvard.edu/~michaelm/postscripts/tr-02-05.pdf
  return hashed_value_1 + hashed_value_2 * coefficient;
}

template <typename T, typename Counter, typename Hash1, typename Hash2>
Counter FilterBloom<T, Counter, Hash1, Hash2>::MinFrequency(
    const HashedType& hashed_value_1, const HashedType& hashed_value_2) const {
  std::optional<Counter> min_count;
  for (std::size_t step = 0; step < kHashFunctionsCount; ++step) {
    auto current_count =
        counters_[GetHash(hashed_value_1, hashed_value_2, Coefficient(step)) %
                  counters_.size()];
    if (!min_count.has_value() || min_count.value() > current_count) {
      min_count = current_count;
    }
  }
  return min_count.value();
}

template <typename T, typename Counter, typename Hash1, typename Hash2>
void FilterBloom<T, Counter, Hash1, Hash2>::Increment(const T& item) {
  auto hash_value_1 = hasher_1(item);
  auto hash_value_2 = hasher_2(item);
  Counter min_frequency = MinFrequency(hash_value_1, hash_value_2);

  for (std::size_t step = 0; step < kHashFunctionsCount; ++step) {
    auto& current_count =
        counters_[GetHash(hash_value_1, hash_value_2, Coefficient(step)) %
                  counters_.size()];
    if (current_count == min_frequency) {
      current_count++;
    }
  }
}

template <typename T, typename Counter, typename Hash1, typename Hash2>
Counter FilterBloom<T, Counter, Hash1, Hash2>::Estimate(const T& item) const {
  auto hash_value_1 = hasher_1(item);
  auto hash_value_2 = hasher_2(item);
  return MinFrequency(hash_value_1, hash_value_2);
}

template <typename T, typename Counter, typename Hash1, typename Hash2>
bool FilterBloom<T, Counter, Hash1, Hash2>::Has(const T& item) const {
  return Estimate(item) > 0;
}

template <typename T, typename Counter, typename Hash1, typename Hash2>
void FilterBloom<T, Counter, Hash1, Hash2>::Clear() {
  for (auto& counter : counters_) {
    counter = 0;
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
