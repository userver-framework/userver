#pragma once

#include <list>
#include <unordered_map>

#include <utils/assert.hpp>

namespace cache {

/// Thread-unsafe LRU cache
template <typename T, typename U, typename Hash = std::hash<T>,
          typename Equal = std::equal_to<T>>
class LRU final {
 public:
  explicit LRU(size_t max_size, const Hash& hash = Hash(),
               const Equal& equal = Equal())
      : max_size_(max_size), map_(max_size, hash, equal) {
    UASSERT(max_size_ > 0);
  }

  LRU(const LRU& lru) = default;

  void Put(const T& key, U value);

  void Erase(const T& key);

  const U* Get(const T& key);

  U GetOr(const T& key, const U& default_value);

  void SetMaxSize(size_t new_max_size);

  void Invalidate();

  /// Call Function(const T&, const U&) for all items
  template <typename Function>
  void VisitAll(Function func) const;

  size_t GetSize() const;

 private:
  using Pair = std::pair<T, U>;
  using Map =
      std::unordered_map<T, typename std::list<Pair>::iterator, Hash, Equal>;

  size_t max_size_;
  Map map_;
  std::list<Pair> list_;
};

template <typename T, typename U, typename Hash, typename Eq>
void LRU<T, U, Hash, Eq>::Put(const T& key, U value) {
  auto it = map_.find(key);
  if (it != map_.end()) {
    it->second->second = std::move(value);
    list_.splice(list_.end(), list_, it->second);
  } else {
    if (map_.size() >= max_size_) {
      auto& pair = list_.front();
      auto node = map_.extract(pair.first);

      auto list_it = list_.begin();
      pair.first = key;
      pair.second = std::move(value);
      list_.splice(list_.end(), list_, list_it);
      node.key() = key;
      node.mapped() = list_it;
      map_.insert(std::move(node));
    } else {
      auto new_it =
          list_.insert(list_.end(), std::make_pair(key, std::move(value)));
      map_.emplace(key, new_it);
    }
  }
}

template <typename T, typename U, typename Hash, typename Eq>
void LRU<T, U, Hash, Eq>::Erase(const T& key) {
  auto it = map_.find(key);
  if (it == map_.end()) return;
  list_.erase(it->second);
  map_.erase(it);
}

template <typename T, typename U, typename Hash, typename Eq>
const U* LRU<T, U, Hash, Eq>::Get(const T& key) {
  auto it = map_.find(key);
  if (it == map_.end()) return nullptr;
  list_.splice(list_.end(), list_, it->second);
  return &it->second->second;
}

template <typename T, typename U, typename Hash, typename Eq>
U LRU<T, U, Hash, Eq>::GetOr(const T& key, const U& default_value) {
  auto ptr = Get(key);
  if (ptr) return *ptr;
  return default_value;
}

template <typename T, typename U, typename Hash, typename Eq>
void LRU<T, U, Hash, Eq>::SetMaxSize(size_t new_max_size) {
  UASSERT(max_size_ > 0);
  max_size_ = new_max_size;
}

template <typename T, typename U, typename Hash, typename Eq>
void LRU<T, U, Hash, Eq>::Invalidate() {
  map_.clear();
  list_.clear();
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename Function>
void LRU<T, U, Hash, Eq>::VisitAll(Function func) const {
  for (auto it : list_) func(it.first, it.second);
}

template <typename T, typename U, typename Hash, typename Eq>
size_t LRU<T, U, Hash, Eq>::GetSize() const {
  return map_.size();
}

}  // namespace cache
