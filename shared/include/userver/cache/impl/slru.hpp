#pragma once

#include <userver/cache/impl/lru.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {
template <typename T, typename U, typename Hash, typename Equal>
class LruBase<T, U, Hash, Equal, CachePolicy::kSLRU> final {
 public:
  explicit LruBase(size_t max_size, const Hash& hash, const Equal& equal,
                   double probation_part = 0.8);

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
  void VisitProbation(Function&& func);
  template <typename Function>
  void VisitProtected(Function&& func);
  template <typename Function>
  void VisitAll(Function&& func);

  size_t GetProbationSize() const { return probation_.GetSize(); }
  size_t GetProtectedSize() const { return protected_.GetSize(); }
  size_t GetSize() const { return GetProbationSize() + GetProtectedSize(); }

 private:
  double probation_part_;
  size_t probation_size_;
  LruBase<T, U> probation_;
  size_t protected_size_;
  LruBase<T, U> protected_;
};

template <typename T, typename U, typename Hash, typename Equal>
LruBase<T, U, Hash, Equal, CachePolicy::kSLRU>::LruBase(size_t max_size,
                                                        const Hash& hash,
                                                        const Equal& equal,
                                                        double probation_part)
    : probation_part_(probation_part),
      probation_size_(std::round(max_size * probation_part)),
      probation_(static_cast<size_t>(std::round(max_size * probation_part)) + 1,
                 hash, equal),
      protected_size_(std::round(max_size * (1. - probation_part))),
      protected_(
          static_cast<size_t>(std::round(max_size * (1 - probation_part))) + 1,
          hash, equal) {}

template <typename T, typename U, typename Hash, typename Eq>
bool LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::Put(const T& key, U value) {
  auto result = probation_.Put(key, value);
  if (!result) {
    probation_.Erase(key);
    result = protected_.Put(key, value);
    if (!result) return false;
    if (protected_.GetSize() == protected_size_ + 1) {
      probation_.Put(*protected_.GetLeastUsedKey(),
                     *protected_.GetLeastUsedValue());
      protected_.Erase(*protected_.GetLeastUsedKey());
    }
    return false;
  }
  if (probation_.GetSize() == probation_size_ + 1)
    probation_.Erase(*probation_.GetLeastUsedKey());
  return true;
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename... Args>
U* LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::Emplace(const T& key,
                                                        Args&&... args) {
  auto* existing = Get(key);
  if (existing) return existing;
  Put(key, U{std::forward<Args>(args)...});
  return probation_.Get(key);
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::Erase(const T& key) {
  probation_.Erase(key);
  protected_.Erase(key);
}

template <typename T, typename U, typename Hash, typename Eq>
U* LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::Get(const T& key) {
  auto result = probation_.Get(key);
  if (!result) return protected_.Get(key);

  protected_.Put(key, *result);
  probation_.Erase(key);
  if (protected_.GetSize() == protected_size_ + 1) {
    probation_.Put(*protected_.GetLeastUsedKey(),
                   *protected_.GetLeastUsedValue());
    protected_.Erase(*protected_.GetLeastUsedKey());
  }
  return protected_.Get(key);
}

template <typename T, typename U, typename Hash, typename Eq>
const T* LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::GetLeastUsedKey() {
  if (probation_.GetSize()) return probation_.GetLeastUsedValue();
  return protected_.GetLeastUsedValue();
}

template <typename T, typename U, typename Hash, typename Eq>
U* LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::GetLeastUsedValue() {
  if (probation_.GetSize()) return probation_.GetLeastUsedKey();
  return protected_.GetLeastUsedKey();
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::SetMaxSize(
    size_t new_max_size) {
  probation_size_ =
      static_cast<size_t>(std::round(new_max_size * probation_part_));
  protected_size_ =
      static_cast<size_t>(std::round(new_max_size * (1. - probation_part_)));
  probation_.SetMaxSize(probation_size_ + 1);
  protected_.SetMaxSize(protected_size_ + 1);
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::Clear() noexcept {
  probation_.Clear();
  protected_.Clear();
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename Function>
void LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::VisitAll(
    Function&& func) const {
  probation_.template VisitAll(func);
  protected_.template VisitAll(std::forward<Function>(func));
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename Function>
void LruBase<T, U, Hash, Equal, CachePolicy::kSLRU>::VisitProbation(
    Function&& func) {
  probation_.template VisitAll(func);
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename Function>
void LruBase<T, U, Hash, Equal, CachePolicy::kSLRU>::VisitProtected(
    Function&& func) {
  protected_.template VisitAll(func);
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename Function>
void LruBase<T, U, Hash, Eq, CachePolicy::kSLRU>::VisitAll(Function&& func) {
  probation_.template VisitAll(func);
  protected_.template VisitAll(std::forward<Function>(func));
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
