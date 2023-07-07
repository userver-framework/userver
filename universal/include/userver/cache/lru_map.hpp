#pragma once

/// @file userver/cache/lru_map.hpp
/// @brief @copybrief cache::LruMap

#include <userver/cache/impl/lru.hpp>

USERVER_NAMESPACE_BEGIN

/// Utilities for caching
namespace cache {

/// @ingroup userver_containers
///
/// LRU key value storage (LRU cache), thread safety matches Standard Library
/// thread safety
template <typename T, typename U, typename Hash = std::hash<T>,
          typename Equal = std::equal_to<T>>
class LruMap final {
 public:
  explicit LruMap(size_t max_size, const Hash& hash = Hash(),
                  const Equal& equal = Equal())
      : impl_(max_size, hash, equal) {}

  LruMap(LruMap&& lru) noexcept = default;
  LruMap(const LruMap& lru) = delete;
  LruMap& operator=(LruMap&& lru) noexcept = default;
  LruMap& operator=(const LruMap& lru) = delete;

  /// Adds or rewrites key/value, updates its usage
  /// @returns true if key is a new one
  bool Put(const T& key, U value) { return impl_.Put(key, std::move(value)); }

  /// Returns pointer to value if the key is in LRU and updates its usage;
  /// constructs and adds a new key/value otherwise.
  /// @warning Returned pointer may be freed on the next map access!
  template <typename... Args>
  U* Emplace(const T& key, Args&&... args) {
    return impl_.Emplace(key, std::forward<Args>(args)...);
  }

  /// Removes key from LRU
  void Erase(const T& key) { impl_.Erase(key); }

  /// Returns pointer to value if the key is in LRU and updates its usage;
  /// returns nullptr otherwise.
  /// @warning Returned pointer may be freed on the next map access!
  U* Get(const T& key) { return impl_.Get(key); }

  /// Returns value by key and updates its usage; returns default_value
  /// otherwise without modifying the cache.
  U GetOr(const T& key, const U& default_value) {
    auto* ptr = impl_.Get(key);
    if (ptr) return *ptr;
    return default_value;
  }

  /// Returns pointer to the least recently used value;
  /// returns nullptr if LRU is empty.
  /// @warning Returned pointer may be freed on the next map access!
  U* GetLeastUsed() { return impl_.GetLeastUsedValue(); }

  /// Sets the max size of the LRU, truncates values if new_max_size < GetSize()
  void SetMaxSize(size_t new_max_size) {
    return impl_.SetMaxSize(new_max_size);
  }

  /// Removes all the elements
  void Clear() { return impl_.Clear(); }

  /// Call Function(const T&, const U&) for all items
  template <typename Function>
  void VisitAll(Function&& func) const {
    impl_.VisitAll(std::forward<Function>(func));
  }

  /// Call Function(const T&, U&) for all items
  template <typename Function>
  void VisitAll(Function&& func) {
    impl_.VisitAll(std::forward<Function>(func));
  }

  size_t GetSize() const { return impl_.GetSize(); }

  std::size_t GetCapacity() const { return impl_.GetCapacity(); }

 private:
  impl::LruBase<T, U, Hash, Equal> impl_;
};

}  // namespace cache

USERVER_NAMESPACE_END
