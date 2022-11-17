#pragma once

#include <utility>

#include <userver/cache/impl/frequency_sketch.hpp>
#include <userver/cache/impl/lru.hpp>
#include <userver/cache/impl/hash.hpp>
#include <userver/cache/policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

template <typename T, typename U, typename Hash, typename Equal>
class LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU> {
 public:
  explicit LruBase(std::size_t max_size, const Hash& hash, const Equal& equal);

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
  void SetMaxSize(std::size_t new_max_size);
  void Clear() noexcept;
  template <typename Function>
  void VisitAll(Function&& func) const;
  template <typename Function>
  void VisitAll(Function&& func);
  std::size_t GetSize() const;

 private:
  FrequencySketch<T, internal::Jenkins<T>, FrequencySketchPolicy::Bloom> proxy_;
  std::size_t max_size_;
  LruBase<T, U, Hash, Equal, CachePolicy::kLRU> main_;
};

// need max_size > 0 for put
template <typename T, typename U, typename Hash, typename Equal>
LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::LruBase(std::size_t max_size,
                                                           const Hash& hash,
                                                           const Equal& equal)
    : proxy_(max_size, internal::Jenkins<T>{}), max_size_(max_size), main_(max_size + 1, hash, equal) {}

// TODO: what should be returned? (same get() or success/failure)
template <typename T, typename U, typename Hash, typename Equal>
bool LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::Put(const T& key,
                                                            U value) {
  proxy_.RecordAccess(key);
  auto res = main_.Put(key, value);
  if (main_.GetSize() == max_size_ + 1) {
    if (proxy_.GetFrequency(key) >
        proxy_.GetFrequency(*main_.GetLeastUsedKey()))
      main_.Erase(*main_.GetLeastUsedKey());
    else
      main_.Erase(key);
  }
  return res;
}
template <typename T, typename U, typename Hash, typename Equal>
template <typename... Args>
U* LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::Emplace(const T& key,
                                                              Args&&... args) {
  auto* existing = main_.Get(key);
  if (existing) return existing;
  Put(key, U{std::forward<Args>(args)...});
  return main_.Get(key);
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

// TODO: think about more smart new_proxy init | need tests
template <typename T, typename U, typename Hash, typename Equal>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::SetMaxSize(
    std::size_t new_max_size) {
  auto new_proxy =
      FrequencySketch<T, internal::Jenkins<T>, FrequencySketchPolicy::Bloom>(new_max_size, internal::Jenkins<T>{});
  main_.VisitAll([&new_proxy](const T& key, const U&) mutable {
    new_proxy.RecordAccess(key);
  });
  proxy_ = new_proxy;
  main_.SetMaxSize(new_max_size + 1);
  max_size_ = new_max_size;
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
std::size_t LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU>::GetSize() const {
  return main_.GetSize();
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
