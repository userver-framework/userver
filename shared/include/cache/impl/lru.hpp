#pragma once

#include <unordered_map>

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>

#include <utils/assert.hpp>

namespace cache::impl {

struct EmptyPlaceholder {};

using LinkMode = boost::intrusive::link_mode<
#ifdef NDEBUG
    boost::intrusive::normal_link
#else
    boost::intrusive::safe_link
#endif
    >;

using LruHook = boost::intrusive::list_base_hook<LinkMode>;

template <class Value>
class LruNode final : public LruHook {
 public:
  explicit LruNode(Value&& value) : value_(std::move(value)) {}

  void SetValue(Value&& value) { value_ = std::move(value); }

  const Value& GetValue() const noexcept { return value_; }

 private:
  Value value_;
};

template <>
class LruNode<EmptyPlaceholder> final : public LruHook {
 public:
  explicit LruNode(EmptyPlaceholder&& /*value*/) {}

  void SetValue(EmptyPlaceholder&& /*value*/) noexcept {}

  const EmptyPlaceholder& GetValue() const noexcept {
    static constexpr EmptyPlaceholder kValue{};
    return kValue;
  }
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
  using LruNode = LruNode<U>;
  using List =
      boost::intrusive::list<LruNode,
                             boost::intrusive::constant_time_size<false>>;
  using Map = std::unordered_map<T, LruNode, Hash, Equal>;

  void MarkRecentlyUsed(LruNode& node) noexcept;
  const T& GetLeastRecentKey();

  size_t max_size_;
  Map map_;
  List list_;
};

template <typename T, typename U, typename Hash, typename Eq>
bool LruBase<T, U, Hash, Eq>::Put(const T& key, U value) {
  auto it = map_.find(key);
  if (it != map_.end()) {
    it->second.SetValue(std::move(value));
    MarkRecentlyUsed(it->second);
    return false;
  }

  if (map_.size() >= max_size_) {
    auto node = map_.extract(GetLeastRecentKey());
    node.key() = key;
    node.mapped().SetValue(std::move(value));
    MarkRecentlyUsed(node.mapped());
    map_.insert(std::move(node));
  } else {
    auto [it, ok] = map_.emplace(key, std::move(value));
    UASSERT(ok);
    list_.insert(list_.end(), it->second);
  }

  return true;
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::Erase(const T& key) {
  auto it = map_.find(key);
  if (it == map_.end()) return;
  list_.erase(list_.iterator_to(it->second));
  map_.erase(it);
}

template <typename T, typename U, typename Hash, typename Eq>
const U* LruBase<T, U, Hash, Eq>::Get(const T& key) {
  auto it = map_.find(key);
  if (it == map_.end()) return nullptr;
  MarkRecentlyUsed(it->second);
  return &it->second.GetValue();
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::SetMaxSize(size_t new_max_size) {
  UASSERT(max_size_ > 0);

  while (map_.size() > new_max_size) {
    auto map_it = map_.find(GetLeastRecentKey());
    UASSERT(map_it != map_.end());
    list_.erase(list_.iterator_to(map_it->second));
    map_.erase(map_it);
  }

  max_size_ = new_max_size;
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::Clear() {
  list_.clear();
  map_.clear();
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename Function>
void LruBase<T, U, Hash, Eq>::VisitAll(Function&& func) const {
  for (const auto& [key, value] : map_) {
    func(key, value.GetValue());
  }
}

template <typename T, typename U, typename Hash, typename Eq>
size_t LruBase<T, U, Hash, Eq>::GetSize() const {
  return map_.size();
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::MarkRecentlyUsed(LruNode& node) noexcept {
  list_.splice(list_.end(), list_, list_.iterator_to(node));
}

template <typename T, typename U, typename Hash, typename Eq>
const T& LruBase<T, U, Hash, Eq>::GetLeastRecentKey() {
  using Pair = typename Map::value_type;

  // Map stores `value_type` and `second` of that `value_type` is an `LruNode`
  // in `list_`.
  constexpr auto offset = offsetof(Pair, second) - offsetof(Pair, first);
  return *reinterpret_cast<const T*>(
      reinterpret_cast<const char*>(&list_.front()) - offset);
}

}  // namespace cache::impl
