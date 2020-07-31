#pragma once

#include <list>
#include <unordered_map>

#include <utils/assert.hpp>

namespace cache::impl {

struct EmptyPlaceholder {};

template <class Key, class Value>
class LruPair {
 public:
  LruPair(Key&& key, Value&& value)
      : key_(std::move(key)), value_(std::move(value)) {}

  void SetValue(Value&& value) { value_ = std::move(value); }

  const Key& GetKey() const noexcept { return key_; }
  const Value& GetValue() const noexcept { return value_; }

 private:
  Key key_;
  Value value_;
};

template <class Key>
class LruPair<Key, EmptyPlaceholder> {
 public:
  LruPair(Key&& key, EmptyPlaceholder&& /*value*/) : key_(std::move(key)) {}

  void SetValue(EmptyPlaceholder&& /*value*/) noexcept {}

  const Key& GetKey() const noexcept { return key_; }

  const EmptyPlaceholder& GetValue() const noexcept {
    static constexpr EmptyPlaceholder kValue{};
    return kValue;
  }

 private:
  Key key_;
};

template <typename T, typename U, typename Hash = std::hash<T>,
          typename Equal = std::equal_to<T>>
class LruBase final {
 public:
  explicit LruBase(size_t max_size, const Hash& hash, const Equal& equal)
      : max_size_(max_size), map_(max_size, hash, equal) {
    UASSERT(max_size_ > 0);
  }

  LruBase(LruBase&& lru) = default;
  LruBase(const LruBase& lru) = delete;
  LruBase& operator=(LruBase&& lru) = default;
  LruBase& operator=(const LruBase& lru) = delete;

  bool Put(const T& key, U value);

  void Erase(const T& key);

  const U* Get(const T& key);

  void SetMaxSize(size_t new_max_size);

  void Clear();

  template <typename Function>
  void VisitAll(Function&& func) const;

  size_t GetSize() const;

 private:
  // Operations on std::unordered_map invalidate iterators, but do not
  // invalidate pointers. Keeping pointers to avoid data duplication
  using Pair = LruPair<const T*, U>;
  using List = std::list<Pair>;
  using Map = std::unordered_map<T, typename List::iterator, Hash, Equal>;

  size_t max_size_;
  Map map_;
  List list_;
};

template <typename T, typename U, typename Hash, typename Eq>
bool LruBase<T, U, Hash, Eq>::Put(const T& key, U value) {
  auto it = map_.find(key);
  if (it != map_.end()) {
    it->second->SetValue(std::move(value));
    list_.splice(list_.end(), list_, it->second);
    return false;
  } else {
    if (map_.size() >= max_size_) {
      auto list_it = list_.begin();
      auto node = map_.extract(*list_it->GetKey());

      list_.splice(list_.end(), list_, list_it);
      node.key() = key;
      UASSERT(list_it->GetKey() == &node.key());
      list_it->SetValue(std::move(value));
      UASSERT(node.mapped() == list_it);
      map_.insert(std::move(node));
    } else {
      auto [it, ok] = map_.emplace(key, typename List::iterator{});
      UASSERT(ok);

      auto new_it =
          list_.insert(list_.end(), Pair(&it->first, std::move(value)));

      it->second = new_it;
    }
  }

  return true;
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::Erase(const T& key) {
  auto it = map_.find(key);
  if (it == map_.end()) return;
  list_.erase(it->second);
  map_.erase(it);
}

template <typename T, typename U, typename Hash, typename Eq>
const U* LruBase<T, U, Hash, Eq>::Get(const T& key) {
  auto it = map_.find(key);
  if (it == map_.end()) return nullptr;
  list_.splice(list_.end(), list_, it->second);
  return &it->second->GetValue();
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::SetMaxSize(size_t new_max_size) {
  UASSERT(max_size_ > 0);

  while (map_.size() > new_max_size) {
    auto it = list_.begin();
    auto map_it = map_.find(*it->GetKey());
    UASSERT(map_it != map_.end());
    list_.erase(it);
    map_.erase(map_it);
  }

  max_size_ = new_max_size;
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::Clear() {
  map_.clear();
  list_.clear();
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename Function>
void LruBase<T, U, Hash, Eq>::VisitAll(Function&& func) const {
  for (const auto& [key, value] : list_) {
    func(key, value);
  }
}

template <typename T, typename U, typename Hash, typename Eq>
size_t LruBase<T, U, Hash, Eq>::GetSize() const {
  return map_.size();
}

}  // namespace cache::impl
