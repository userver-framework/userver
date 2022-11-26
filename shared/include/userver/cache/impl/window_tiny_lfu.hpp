#pragma once

#include <userver/cache/impl/tiny_lfu.hpp>
#include <userver/cache/policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {
template <typename T, typename U, typename Hash, typename Equal>
class LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU> {
 public:
  explicit LruBase(std::size_t max_size, const Hash& hash, const Equal& equal,
                   double window_part = 0.01);

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
  size_t GetWindowSize() const { return window_.GetSize(); }
  size_t GetMainSize() const { return main_.GetSize(); }
  size_t GetSize() const { return GetWindowSize() + GetMainSize(); }

 private:
  double window_part_;
  size_t window_size_;
  LruBase<T, U, Hash, Equal, CachePolicy::kLRU> window_;
  size_t main_size_;
  TinyLFU<T, U, Hash, Equal, CachePolicy::kSLRU> main_;
};

template <typename T, typename U, typename Hash, typename Equal>
LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::LruBase(
    std::size_t max_size, const Hash& hash, const Equal& equal,
    double window_part)
    : window_part_(window_part),
      window_size_(std::round(max_size * window_part) + 1),
      window_(std::round(max_size * window_part) + 1, hash, equal),
      main_size_(std::round(max_size * (1. - window_part))),
      main_(std::round(max_size * (1. - window_part)), hash, equal) {}

// TODO: size = 0
// Access to bloom twice?? -- doorkeeper
template <typename T, typename U, typename Hash, typename Equal>
bool LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::Put(const T& key,
                                                             U value) {
  if (window_.GetSize() == window_size_) {
    if (window_.Get(key)) {
      main_.Access(key);
      window_.Put(key, std::move(value));
      return false;
    }
    // TODO: move?
    main_.Put(*window_.GetLeastUsedKey(), *window_.GetLeastUsedValue());
    window_.Put(key, std::move(value));
    return true;
  } else {
    main_.Access(key);
    return window_.Put(key, std::move(value));
  }
}

template <typename T, typename U, typename Hash, typename Equal>
U* LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::Get(const T& key) {
  auto* result = window_.Get(key);
  if (!result) return main_.Get(key);
  main_.Access(key);
  return result;
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename... Args>
U* LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::Emplace(const T& key,
                                                               Args&&... args) {
  auto* existing = Get(key);
  if (existing) return existing;
  Put(key, U{std::forward<Args>(args)...});
  return main_.Get(key);
}

template <typename T, typename U, typename Hash, typename Equal>
void LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::Erase(const T& key) {
  window_.Erase(key);
  main_.Erase(key);
}

template <typename T, typename U, typename Hash, typename Equal>
const T* LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::GetLeastUsedKey() {
  return main_.GetLeastUsedKey();
}

template <typename T, typename U, typename Hash, typename Equal>
U* LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::GetLeastUsedValue() {
  return main_.GetLeastUsedValue();
}

template <typename T, typename U, typename Hash, typename Equal>
void LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::SetMaxSize(
    std::size_t new_max_size) {
  window_size_ = std::round(new_max_size * window_part_) + 1;
  main_size_ = std::round(new_max_size * (1 - window_part_));
  window_.SetMaxSize(window_size_);
  main_.SetMaxSize(main_size_);
}

template <typename T, typename U, typename Hash, typename Equal>
void LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::Clear() noexcept {
  window_.Clear();
  main_.Clear();
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename Function>
void LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::VisitAll(
    Function&& func) const {
  window_.VisitAll(func);
  main_.VisitAll(std::forward<Function>(func));
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename Function>
void LruBase<T, U, Hash, Equal, CachePolicy::kWTinyLFU>::VisitAll(
    Function&& func) {
  window_.VisitAll(func);
  main_.VisitAll(std::forward<Function>(func));
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
