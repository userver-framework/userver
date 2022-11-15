#pragma once

#include <userver/cache/impl/frequency_sketch.hpp>
#include <userver/cache/impl/lru.hpp>
#include <userver/cache/policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

template <typename T, typename U, typename Hash, typename Equal>
class LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU> {
 public:
  explicit LruBase(size_t max_size, const Hash& hash, const Equal& equal);

  LruBase(LruBase&& other) noexcept = default;
  LruBase& operator=(LruBase&& other) noexcept = default;
  LruBase(const LruBase& lru) = delete;
  LruBase& operator=(const LruBase& lru) = delete;

  bool Put(const T& key, U value);
  template <typename... Args>
  U* Emplace(const T&, Args&&... args);
  void Erase(const T& key);
  U* Get(const T& key);
  const T* GetLeastUsedKey();
  U* GetLeastUsedValue();
  void SetMaxSize(size_t new_max_size);
  void Clear() noexcept;
  template <typename Function>
  void VisitAll(Function&& func) const;
  template <typename Function>
  void VisitAll(Function&& func);
  size_t GetSize() const;

 private:
  FrequencySketch<T, FrequencySketchPolicy::Bloom> proxy_;
  LruBase<T, U, Hash, Equal, CachePolicy::kLRU> main_;
};

template <typename T, typename U, typename Hash, typename Equal>
LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::LruBase(size_t max_size,
                                                           const Hash& hash,
                                                           const Equal& equal)
    : main_(max_size, hash, equal), proxy_(max_size) {}

}  // namespace cache::impl

USERVER_NAMESPACE_END
