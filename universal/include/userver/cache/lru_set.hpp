#pragma once

/// @file userver/cache/lru_set.hpp
/// @brief @copybrief cache::LruSet

#include <userver/cache/impl/lru.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

/// @ingroup userver_containers
///
/// LRU set, thread safety matches Standard Library thread safety
template <typename T, typename Hash = std::hash<T>,
          typename Equal = std::equal_to<T>>
class LruSet final {
 public:
  explicit LruSet(size_t max_size, const Hash& hash = Hash(),
                  const Equal& equal = Equal())
      : impl_(max_size, hash, equal) {}

  LruSet(LruSet&& lru) noexcept = default;
  LruSet(const LruSet& lru) = delete;
  LruSet& operator=(LruSet&& lru) noexcept = default;
  LruSet& operator=(const LruSet& lru) = delete;

  /// Adds key to the LRU or updates its usage
  /// @returns true if key is a new one
  bool Put(const T& key) { return impl_.Put(key, {}); }

  /// Removes key from LRU
  void Erase(const T& key) { return impl_.Erase(key); }

  /// Checks whether the key in LRU and updates its usage
  bool Has(const T& key) { return impl_.Get(key); }

  /// Sets the max size of the LRU, truncates values if new_max_size < GetSize()
  void SetMaxSize(size_t new_max_size) {
    return impl_.SetMaxSize(new_max_size);
  }

  /// Removes all the elements
  void Invalidate() { return impl_.Invalidate(); }

  /// Call Function(const T&) for all items
  template <typename Function>
  void VisitAll(Function&& func) const {
    impl_.VisitAll(
        [&func](const auto& key, const auto& /*value*/) mutable { func(key); });
  }

  size_t GetSize() const { return impl_.GetSize(); }

  /// Returns pointer to the least recently used element
  /// or nullptr if LruSet is empty.
  /// @warning Returned pointer may be freed on the next set access!
  const T* GetLeastUsed() { return impl_.GetLeastUsedKey(); }

 private:
  impl::LruBase<T, impl::EmptyPlaceholder, Hash, Equal> impl_;
};

}  // namespace cache

USERVER_NAMESPACE_END
