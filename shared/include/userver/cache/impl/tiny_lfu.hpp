#pragma once

#include <utility>

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
  U* Emplace(const T& key, Args&&... args);
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

template <typename T, typename U, typename Hash, typename Equal>
bool LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::Put(const T& key,
                                                            U value) {
  //// getting ptr on key which will be deleted if new key is added;
  auto least_used_key = main_.GetLeastUsedKey();
  //// new key will be added if only frequency of new key is bigger than
  /// frequency of key which will be deleted
  if (proxy_.GetFrequency(*least_used_key) < proxy_.GetFrequency(key)) {
    proxy_.RecordAccess(key);
    return main_.Put(key, value);
  }
  return false;
}
template <typename T, typename U, typename Hash, typename Equal>
template <typename... Args>
U* LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::Emplace(const T& key,
                                                              Args&&... args) {
  auto least_used_key = main_.GetLeastUsedKey();
  if (proxy_.GetFrequency(*least_used_key) < proxy_.GetFrequency(key)) {
    proxy_.RecordAccess(key);
    return main_.Emplace(key, std::forward<Args>(args)...);
  }
  return nullptr;
}
template <typename T, typename U, typename Hash, typename Equal>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::Erase(const T& key) {
  return main_.Erase(key);
}

template <typename T, typename U, typename Hash, typename Equal>
U* LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::Get(const T& key) {
  proxy_.RecordAccess(key);
  return main_.Get(key);
}

template <typename T, typename U, typename Hash, typename Equal>
const T* LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::GetLeastUsedKey() {
  return main_.GetLeastUsedKey();
}

template <typename T, typename U, typename Hash, typename Equal>
U* LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::GetLeastUsedValue() {
  return main_.GetLeastUsedValue();
}

template <typename T, typename U, typename Hash, typename Equal>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::SetMaxSize(
    size_t new_max_size) {
  auto new_proxy =
      FrequencySketch<T, FrequencySketchPolicy::Bloom>(new_max_size);
  main_.VisitAll([&new_proxy](const T& key, const U& value) mutable {
    new_proxy.RecordAccess(key);
  });
  proxy_ = new_proxy;
  main_.SetMaxSize(new_max_size);
}

template <typename T, typename U, typename Hash, typename Equal>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::Clear() noexcept {
  proxy_.Clear();
  main_.Clear();
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename Function>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::VisitAll(
    Function&& func) const {
  return main_.VisitAll(std::forward<Function>(func));
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename Function>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::VisitAll(
    Function&& func) {
  return main_.VisitAll(std::forward<Function>(func));
}

template <typename T, typename U, typename Hash, typename Equal>
size_t LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::GetSize() const {
  return main_.GetSize();
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
