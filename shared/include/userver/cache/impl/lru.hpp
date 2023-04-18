#pragma once

#include <memory>
#include <vector>

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/unordered_set_hook.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

struct EmptyPlaceholder {};

using LinkMode = utils::impl::IntrusiveLinkMode;
using LruListHook = boost::intrusive::list_base_hook<LinkMode>;
using LruHashSetHook = boost::intrusive::unordered_set_base_hook<LinkMode>;

template <class Key, class Value>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class LruNode final : public LruListHook, public LruHashSetHook {
 public:
  template <typename... Args>
  explicit LruNode(Key&& key, Args&&... args)
      : key_(std::move(key)), value_(std::forward<Args>(args)...) {}

  explicit LruNode(Key&& key, Value&& value)
      : key_(std::move(key)), value_(std::move(value)) {}

  void SetKey(Key key) { key_ = std::move(key); }

  void SetValue(Value&& value) { value_ = std::move(value); }

  const Key& GetKey() const noexcept { return key_; }

  const Value& GetValue() const noexcept { return value_; }
  Value& GetValue() noexcept { return value_; }

 private:
  Key key_;
  Value value_;
};

template <class Key>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class LruNode<Key, EmptyPlaceholder> final : public LruListHook,
                                             public LruHashSetHook {
 public:
  explicit LruNode(Key&& key, EmptyPlaceholder /*value*/)
      : key_(std::move(key)) {}

  void SetKey(Key key) { key_ = std::move(key); }

  void SetValue(EmptyPlaceholder /*value*/) noexcept {}

  const Key& GetKey() const noexcept { return key_; }

  EmptyPlaceholder& GetValue() const noexcept {
    static EmptyPlaceholder kValue{};
    return kValue;
  }

 private:
  Key key_;
};

template <class Key, class Value>
const Key& GetKey(const LruNode<Key, Value>& node) noexcept {
  return node.GetKey();
}

template <class T>
const T& GetKey(const T& key) noexcept {
  return key;
}

template <typename T, typename U, typename Hash = std::hash<T>,
          typename Equal = std::equal_to<T>>
class LruBase final {
 public:
  using NodeType = std::unique_ptr<LruNode<T, U>>;

  explicit LruBase(size_t max_size, const Hash& hash, const Equal& equal);
  ~LruBase() { Clear(); }

  LruBase(LruBase&& other) noexcept
      : buckets_(std::move(other.buckets_)),
        map_(std::move(other.map_)),
        list_(std::move(other.list_)) {
    other.buckets_.clear();
    other.map_.clear();
    other.list_.clear();
  }

  LruBase& operator=(LruBase&& other) noexcept {
    if (this != &other) Clear();

    swap(other.buckets_, buckets_);
    swap(other.map_, map_);
    swap(other.list_, list_);

    return *this;
  }

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
  void VisitAll(Function&& func);

  size_t GetSize() const;

  U& InsertNode(NodeType&& node) noexcept;
  NodeType ExtractNode(const T& key) noexcept;

  std::size_t GetCapacity() const;

 private:
  using Node = LruNode<T, U>;
  using List =
      boost::intrusive::list<Node, boost::intrusive::constant_time_size<false>>;

  struct LruNodeHash : Hash {
    LruNodeHash(const Hash& h) : Hash{h} {}

    template <class NodeOrKey>
    auto operator()(const NodeOrKey& x) const {
      return Hash::operator()(impl::GetKey(x));
    }
  };

  struct LruNodeEqual : Equal {
    LruNodeEqual(const Equal& eq) : Equal{eq} {}

    template <class NodeOrKey1, class NodeOrKey2>
    auto operator()(const NodeOrKey1& x, const NodeOrKey2& y) const {
      return Equal::operator()(impl::GetKey(x), impl::GetKey(y));
    }
  };

  using Map = boost::intrusive::unordered_set<
      Node, boost::intrusive::constant_time_size<true>,
      boost::intrusive::hash<LruNodeHash>,
      boost::intrusive::equal<LruNodeEqual>>;

  using BucketTraits = typename Map::bucket_traits;
  using BucketType = typename Map::bucket_type;

  U& Add(const T& key, U value);
  void MarkRecentlyUsed(Node& node) noexcept;
  std::unique_ptr<Node> ExtractNode(typename List::iterator it) noexcept;

  std::vector<BucketType> buckets_;
  Map map_;
  List list_;
};

template <typename T, typename U, typename Hash, typename Equal>
LruBase<T, U, Hash, Equal>::LruBase(size_t max_size, const Hash& hash,
                                    const Equal& eq)
    : buckets_(max_size ? max_size : 1),
      map_(BucketTraits(buckets_.data(), buckets_.size()), hash, eq) {
  UASSERT(max_size > 0);
}

template <typename T, typename U, typename Hash, typename Eq>
bool LruBase<T, U, Hash, Eq>::Put(const T& key, U value) {
  auto it = map_.find(key, map_.hash_function(), map_.key_eq());
  if (it != map_.end()) {
    it->SetValue(std::move(value));
    MarkRecentlyUsed(*it);
    return false;
  }

  Add(key, std::move(value));
  return true;
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename... Args>
U* LruBase<T, U, Hash, Eq>::Emplace(const T& key, Args&&... args) {
  auto* existing = Get(key);
  if (existing) return existing;

  if constexpr (std::is_move_assignable_v<U>) {
    return &Add(key, U{std::forward<Args>(args)...});
  } else {
    auto node = std::make_unique<Node>(T{key}, std::forward<Args>(args)...);
    if (map_.size() >= buckets_.size()) {
      ExtractNode(list_.begin());
    }
    return &InsertNode(std::move(node));
  }
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::Erase(const T& key) {
  auto it = map_.find(key, map_.hash_function(), map_.key_eq());
  if (it == map_.end()) return;
  ExtractNode(list_.iterator_to(*it));
}

template <typename T, typename U, typename Hash, typename Eq>
U* LruBase<T, U, Hash, Eq>::Get(const T& key) {
  auto it = map_.find(key, map_.hash_function(), map_.key_eq());
  if (it == map_.end()) return nullptr;
  MarkRecentlyUsed(*it);
  return &it->GetValue();
}

template <typename T, typename U, typename Hash, typename Eq>
const T* LruBase<T, U, Hash, Eq>::GetLeastUsedKey() {
  if (list_.empty()) return nullptr;
  return &list_.front().GetKey();
}

template <typename T, typename U, typename Hash, typename Eq>
U* LruBase<T, U, Hash, Eq>::GetLeastUsedValue() {
  if (list_.empty()) return nullptr;
  return &list_.front().GetValue();
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::SetMaxSize(size_t new_max_size) {
  UASSERT(new_max_size > 0);
  if (!new_max_size) ++new_max_size;

  if (buckets_.size() == new_max_size) {
    return;
  }

  while (map_.size() > new_max_size) {
    ExtractNode(list_.begin());
  }

  std::vector<BucketType> new_buckets(new_max_size);
  map_.rehash(BucketTraits(new_buckets.data(), new_max_size));
  buckets_.swap(new_buckets);
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::Clear() noexcept {
  while (!list_.empty()) {
    ExtractNode(list_.begin());
  }
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename Function>
void LruBase<T, U, Hash, Eq>::VisitAll(Function&& func) const {
  for (const auto& node : map_) {
    func(node.GetKey(), node.GetValue());
  }
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename Function>
void LruBase<T, U, Hash, Eq>::VisitAll(Function&& func) {
  for (auto& node : map_) {
    func(node.GetKey(), node.GetValue());
  }
}

template <typename T, typename U, typename Hash, typename Eq>
size_t LruBase<T, U, Hash, Eq>::GetSize() const {
  return map_.size();
}

template <typename T, typename U, typename Hash, typename Eq>
std::size_t LruBase<T, U, Hash, Eq>::GetCapacity() const {
  return buckets_.size();
}

template <typename T, typename U, typename Hash, typename Eq>
U& LruBase<T, U, Hash, Eq>::Add(const T& key, U value) {
  if (map_.size() < buckets_.size()) {
    auto node = std::make_unique<Node>(T{key}, std::move(value));
    return InsertNode(std::move(node));
  }

  auto node = ExtractNode(list_.begin());
  node->SetKey(key);
  node->SetValue(std::move(value));
  return InsertNode(std::move(node));
}

template <typename T, typename U, typename Hash, typename Eq>
void LruBase<T, U, Hash, Eq>::MarkRecentlyUsed(Node& node) noexcept {
  list_.splice(list_.end(), list_, list_.iterator_to(node));
}

template <typename T, typename U, typename Hash, typename Eq>
std::unique_ptr<LruNode<T, U>> LruBase<T, U, Hash, Eq>::ExtractNode(
    typename List::iterator it) noexcept {
  UASSERT(it != list_.end());

  std::unique_ptr<Node> ret(&*it);
  map_.erase(map_.iterator_to(*it));
  list_.erase(it);
  return ret;
}

template <typename T, typename U, typename Hash, typename Eq>
U& LruBase<T, U, Hash, Eq>::InsertNode(
    LruBase<T, U, Hash, Eq>::NodeType&& node) noexcept {
  UASSERT(node);

  auto [it, ok] = map_.insert(*node);  // noexcept
  UASSERT(ok);
  list_.insert(list_.end(), *node);  // noexcept

  return node.release()->GetValue();
}

template <typename T, typename U, typename Hash, typename Eq>
typename LruBase<T, U, Hash, Eq>::NodeType LruBase<T, U, Hash, Eq>::ExtractNode(
    const T& key) noexcept {
  auto it = map_.find(key);
  if (it == map_.end()) {
    return std::unique_ptr<typename LruBase<T, U, Hash, Eq>::NodeType>();
  }

  return ExtractNode(list_.iterator_to(*it));
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
