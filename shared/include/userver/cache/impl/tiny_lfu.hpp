#pragma once

#include <utility>

#include <userver/cache/impl/frequency_sketch.hpp>
#include <userver/cache/impl/hash.hpp>
#include <userver/cache/impl/lru.hpp>
#include <userver/cache/impl/slru.hpp>
#include <userver/cache/policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
class LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, InnerPolicy> {
  using DefaultBloomHash = tools::Jenkins<T>;
  static constexpr auto DefaultFrequencySketchPolicy =
      FrequencySketchPolicy::DoorkeeperBloom;

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
  FrequencySketch<T, DefaultBloomHash, DefaultFrequencySketchPolicy> proxy_;
  std::size_t max_size_;
  LruBase<T, U, Hash, Equal, InnerPolicy> main_;
};

template <typename T, typename U, typename Hash, typename Equal>
class LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU> {
 public:
  explicit LruBase(std::size_t max_size, const Hash& hash, const Equal& equal)
      : inner_(max_size, hash, equal) {}

  LruBase(LruBase&& other) noexcept = default;
  LruBase& operator=(LruBase&& other) noexcept = default;
  LruBase(const LruBase& lru) = delete;
  LruBase& operator=(const LruBase& lru) = delete;

  bool Put(const T& key, U value) { return inner_.Put(key, value); }
  template <typename... Args>
  U* Emplace(const T& key, Args&&... args) {
    return inner_.Emplace(key, args...);
  }
  void Erase(const T& key) { inner_.Erase(key); }
  U* Get(const T& key) { return inner_.Get(key); }
  const T* GetLeastUsedKey() { return inner_.GetLeastUsedKey(); }
  U* GetLeastUsedValue() { return inner_.GetLeastUsedValue(); }
  void SetMaxSize(std::size_t new_max_size) { inner_.SetMaxSize(new_max_size); }
  void Clear() noexcept { inner_.Clear(); }
  template <typename Function>
  void VisitAll(Function&& func) const {
    inner_.VisitAll(std::forward<Function>(func));
  }
  template <typename Function>
  void VisitAll(Function&& func) {
    inner_.VisitAll(std::forward<Function>(func));
  }
  std::size_t GetSize() const { return inner_.GetSize(); }

 private:
  LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, CachePolicy::kLRU> inner_;
};

// need max_size > 0 for put
template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, InnerPolicy>::LruBase(
    std::size_t max_size, const Hash& hash, const Equal& equal)
    : proxy_(max_size, DefaultBloomHash{}),
      max_size_(max_size),
      main_(max_size, hash, equal) {}

// TODO: what should be returned? (same get() or success/failure)
// TODO: size 0
template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
bool LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, InnerPolicy>::Put(
    const T& key, U value) {
  proxy_.RecordAccess(key);
  if (main_.GetSize() == max_size_) {
    if (main_.Get(key)) {
      main_.Put(key, std::move(value));
      return false;
    }
    if (proxy_.GetFrequency(key) >
        proxy_.GetFrequency(*main_.GetLeastUsedKey()))
      main_.Put(key, std::move(value));
    return true;
  } else
    return main_.Put(key, std::move(value));
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
template <typename... Args>
U* LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, InnerPolicy>::Emplace(
    const T& key, Args&&... args) {
  auto* existing = main_.Get(key);
  if (existing) return existing;
  Put(key, U{std::forward<Args>(args)...});
  return main_.Get(key);
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, InnerPolicy>::Erase(
    const T& key) {
  main_.Erase(key);
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
U* LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, InnerPolicy>::Get(
    const T& key) {
  proxy_.RecordAccess(key);
  return main_.Get(key);
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
const T* LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU,
                 InnerPolicy>::GetLeastUsedKey() {
  return main_.GetLeastUsedKey();
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
U* LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU,
           InnerPolicy>::GetLeastUsedValue() {
  return main_.GetLeastUsedValue();
}

// TODO: think about more smart new_proxy init | need tests
template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, InnerPolicy>::SetMaxSize(
    std::size_t new_max_size) {
  auto new_proxy =
      FrequencySketch<T, DefaultBloomHash, DefaultFrequencySketchPolicy>(
          new_max_size, DefaultBloomHash{});
  main_.VisitAll([&new_proxy](const T& key, const U&) mutable {
    new_proxy.RecordAccess(key);
  });
  proxy_ = new_proxy;
  main_.SetMaxSize(new_max_size);
  max_size_ = new_max_size;
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU,
             InnerPolicy>::Clear() noexcept {
  proxy_.Clear();
  main_.Clear();
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
template <typename Function>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, InnerPolicy>::VisitAll(
    Function&& func) const {
  main_.VisitAll(std::forward<Function>(func));
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
template <typename Function>
void LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU, InnerPolicy>::VisitAll(
    Function&& func) {
  main_.VisitAll(std::forward<Function>(func));
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy>
std::size_t LruBase<T, U, Hash, Equal, CachePolicy::kTinyLFU,
                    InnerPolicy>::GetSize() const {
  return main_.GetSize();
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
