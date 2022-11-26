#pragma once

#include <functional>
#include <utility>

#include <userver/cache/impl/hash.hpp>
#include <userver/cache/impl/lru.hpp>
#include <userver/cache/impl/sketch/aged.hpp>
#include <userver/cache/impl/sketch/policy.hpp>
#include <userver/cache/impl/slru.hpp>
#include <userver/cache/policy.hpp>
#include "userver/utils/assert.hpp"

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

// Impl LruBase interface
template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy = CachePolicy::kLRU,
          typename SketchHash = std::hash<T>,
          sketch::Policy SketchPolicy = sketch::Policy::Aged>
class TinyLFU {
  // std::hash -- Bad for digits, good for strings
  // tools::Jenkins -- too slow
 public:
  explicit TinyLFU(std::size_t max_size, const Hash& hash, const Equal& equal);

  TinyLFU(TinyLFU&& other) noexcept = default;
  TinyLFU& operator=(TinyLFU&& other) noexcept = default;
  TinyLFU(const TinyLFU& lru) = delete;
  TinyLFU& operator=(const TinyLFU& lru) = delete;

  bool Put(const T& key, U value);

  template <typename... Args>
  U* Emplace(const T& key, Args&&... args);

  void Erase(const T& key) { main_.Erase(key); }

  U* Get(const T& key) {
    counters_.Increment(key);
    return main_.Get(key);
  }

  const T* GetLeastUsedKey() { return main_.GetLeastUsedKey(); }

  U* GetLeastUsedValue() { return main_.GetLeastUsedValue(); }

  void SetMaxSize(std::size_t new_max_size);

  void Clear() noexcept {
    counters_.Clear();
    main_.Clear();
  }

  template <typename Function>
  void VisitAll(Function&& func) const {
    main_.VisitAll(std::forward<Function>(func));
  }

  template <typename Function>
  void VisitAll(Function&& func) {
    main_.VisitAll(std::forward<Function>(func));
  }

  std::size_t GetSize() const { return main_.GetSize(); }

  // TODO: remove it (slug)
  void Access(const T& key) { counters_.Increment(key); }

 private:
  sketch::Sketch<T, SketchHash, SketchPolicy> counters_;
  std::size_t max_size_;
  LruBase<T, U, Hash, Equal, InnerPolicy> main_;
};

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy, typename SketchHash,
          sketch::Policy SketchPolicy>
TinyLFU<T, U, Hash, Equal, InnerPolicy, SketchHash, SketchPolicy>::TinyLFU(
    std::size_t max_size, const Hash& hash, const Equal& equal)
    : counters_(max_size, SketchHash{}),
      max_size_(max_size),
      main_(max_size, hash, equal) {
  UASSERT(max_size > 0);
}

// TODO: what should be returned? (same get() or success/failure)
// TODO: size 0
template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy, typename SketchHash,
          sketch::Policy SketchPolicy>
bool TinyLFU<T, U, Hash, Equal, InnerPolicy, SketchHash, SketchPolicy>::Put(
    const T& key, U value) {
  counters_.Increment(key);
  if (main_.GetSize() == max_size_) {
    // Update
    if (main_.Get(key)) {
      main_.Put(key, std::move(value));
      return false;
    }
    // Winner
    if (counters_.Estimate(key) > counters_.Estimate(*main_.GetLeastUsedKey()))
      main_.Put(key, std::move(value));
    return true;
  } else
    return main_.Put(key, std::move(value));
}

template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy, typename SketchHash,
          sketch::Policy SketchPolicy>
template <typename... Args>
U* TinyLFU<T, U, Hash, Equal, InnerPolicy, SketchHash, SketchPolicy>::Emplace(
    const T& key, Args&&... args) {
  auto* existing = main_.Get(key);
  if (existing) return existing;
  Put(key, U{std::forward<Args>(args)...});
  return main_.Get(key);
}

// TODO: counters copy-constructor
// TODO: set max size to SketchPolicy
template <typename T, typename U, typename Hash, typename Equal,
          CachePolicy InnerPolicy, typename SketchHash,
          sketch::Policy SketchPolicy>
void TinyLFU<T, U, Hash, Equal, InnerPolicy, SketchHash,
             SketchPolicy>::SetMaxSize(std::size_t new_max_size) {
  UASSERT(new_max_size > 0);
  auto new_counters =
      sketch::Sketch<T, SketchHash, SketchPolicy>(new_max_size, SketchHash{});
  main_.VisitAll([&new_counters](const T& key, const U&) mutable {
    new_counters.Increment(key);
  });
  counters_ = new_counters;
  main_.SetMaxSize(new_max_size);
  max_size_ = new_max_size;
}

// Default
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
  TinyLFU<T, U, Hash, Equal, CachePolicy::kLRU> inner_;
};

}  // namespace cache::impl

USERVER_NAMESPACE_END
