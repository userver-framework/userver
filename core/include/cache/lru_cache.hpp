#pragma once

#include <list>
#include <unordered_map>

#include <utils/assert.hpp>

namespace cache {

/// Thread-unsafe LRU cache
template <typename T, typename U>
class LRU final {
 public:
  explicit LRU(size_t max_size) : max_size_(max_size), map_(max_size) {
    UASSERT(max_size_ > 0);
  }

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
  using Map = std::unordered_map<T, typename std::list<Pair>::iterator>;

  size_t max_size_;
  Map map_;
  std::list<Pair> list_;
};

template <typename T, typename U>
void LRU<T, U>::Put(const T& key, U value) {
  auto it = map_.find(key);
  if (it != map_.end()) {
    it->second->second = std::move(value);
    list_.splice(list_.end(), list_, it->second);
  } else {
    if (map_.size() >= max_size_) {
      auto& pair = list_.front();
      map_.erase(pair.first);

      auto list_it = list_.begin();
      pair.first = key;
      pair.second = std::move(value);
      list_.splice(list_.end(), list_, list_it);
      map_.emplace(key, list_it);
    } else {
      auto new_it =
          list_.insert(list_.end(), std::make_pair(key, std::move(value)));
      map_.emplace(key, new_it);
    }
  }
}

template <typename T, typename U>
void LRU<T, U>::Erase(const T& key) {
  auto it = map_.find(key);
  if (it == map_.end()) return;
  list_.erase(it->second);
  map_.erase(it);
}

template <typename T, typename U>
const U* LRU<T, U>::Get(const T& key) {
  auto it = map_.find(key);
  if (it == map_.end()) return nullptr;
  list_.splice(list_.end(), list_, it->second);
  return &it->second->second;
}

template <typename T, typename U>
U LRU<T, U>::GetOr(const T& key, const U& default_value) {
  auto ptr = Get(key);
  if (ptr) return *ptr;
  return default_value;
}

template <typename T, typename U>
void LRU<T, U>::SetMaxSize(size_t new_max_size) {
  UASSERT(max_size_ > 0);
  max_size_ = new_max_size;
}

template <typename T, typename U>
void LRU<T, U>::Invalidate() {
  map_.clear();
  list_.clear();
}

template <typename T, typename U>
template <typename Function>
void LRU<T, U>::VisitAll(Function func) const {
  for (auto it : list_) func(it.first, it.second);
}

template <typename T, typename U>
size_t LRU<T, U>::GetSize() const {
  return map_.size();
}

}  // namespace cache
