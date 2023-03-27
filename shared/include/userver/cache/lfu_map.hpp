#pragma once

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/unordered_set_hook.hpp>
#include <boost/unordered_map.hpp>
#include <list>
#include <memory>
#include <tuple>
#include <unordered_map>

using LinkMode = boost::intrusive::link_mode<boost::intrusive::safe_link>;
using LfuListHook = boost::intrusive::list_base_hook<LinkMode>;
using LfuHashSetHook = boost::intrusive::unordered_set_base_hook<LinkMode>;

template <typename Value>
struct ValueFreq {
  ValueFreq(Value&& val, size_t freq = 1)
      : value(std::move(val)), frequently(freq) {}
  Value value;
  size_t frequently;
  Value& GetValue() noexcept { return value; }
  const Value& GetValue() const noexcept { return value; }
  size_t GetFrequently() noexcept { return frequently; }
  size_t Increase() noexcept { return ++frequently; }
  size_t Decrease() noexcept { return --frequently; }
};

template <typename Key, typename Value>
class LfuNode final : public LfuHashSetHook {
 public:
  explicit LfuNode(Key&& key, Value&& value)
      : key_(std::move(key)), value_(ValueFreq(std::move(value))) {}
  void SetKey(Key key) { key_ = std::move<Key>(key); }
  void SetValue(Value&& value) { value_ = ValueFreq<Value>(std::move(value)); }
  const Key& GetKey() const noexcept { return key_; }
  const Value& GetValue() const noexcept { return value_.GetValue(); }
  Value& GetValue() noexcept { return value_.GetValue(); }
  size_t GetFrequently() const noexcept { return value_.GetFrequently(); }
  size_t Increase() noexcept { return value_.Increase(); };
  size_t Decrease() noexcept { return value_.Decrease(); };

 private:
  Key key_;
  ValueFreq<Value> value_;
};

template <class Key, class Value>
const Key& GetKey(const LfuNode<Key, Value>& node) noexcept {
  return node.GetKey();
}

template <class T>
const T& GetKey(const T& key) noexcept {
  return key;
}

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<>>
class LfuBase final {
 public:
  explicit LfuBase(size_t max_size, const Hash& hash = Hash(),
                   const Equal& equal = Equal());
  LfuBase(LfuBase&& other) noexcept
      : m_size_(other.m_size_),
        min_freq_(other.min_freq_),
        map_(std::move(other.map_)),
        m_iter_(std::move(other.m_iter_)),
        m_freq_(std::move(other.m_freq_)) {
    other.Clear();
  }
  ~LfuBase() { Clear(); }
  LfuBase& operator=(LfuBase&& other) noexcept {
    if (this != &other) {
      Clear();
    }
    std::swap(other.m_size_, m_size_);
    std::swap(other.min_freq_, min_freq_);
    std::swap(other.buckets_, buckets_);
    std::swap(other.map_, map_);
  }
  void Clear();
  bool Put(const Key& key, Value value);
  template <typename... Args>
  Value* Emplace(const Key& key, Args&&... args);
  //  void Erase(const Key& key);
  Value* Get(const Key& key);
  void SetMaxSize(size_t new_max_size);

  size_t GetSize() const;

  LfuBase(const LfuBase&) = delete;
  LfuBase& operator=(const LfuBase&) = delete;

  Value& Add(const Key& key, Value value);
  LfuNode<Key, Value>& InsertNodeUpdateCounter(
      std::unique_ptr<LfuNode<Key, Value>>&& node) noexcept;

 private:
  struct LfuNodeHash : Hash {
    LfuNodeHash(const Hash& h) : Hash{h} {}

    template <class NodeOrKey>
    auto operator()(const NodeOrKey& x) const {
      return Hash::operator()(GetKey(x));
    }
  };
  struct LfuNodeEqual : Equal {
    LfuNodeEqual(const Equal& eq) : Equal{eq} {}

    template <class NodeOrKey1, class NodeOrKey2>
    auto operator()(const NodeOrKey1& x, const NodeOrKey2& y) const {
      return Equal::operator()(GetKey(x), GetKey(y));
    }
  };

  using Map = boost::intrusive::unordered_set<
      LfuNode<Key, Value>, boost::intrusive::constant_time_size<true>,
      boost::intrusive::hash<LfuNodeHash>,
      boost::intrusive::equal<LfuNodeEqual>>;

  using BucketTraits = typename Map::bucket_traits;
  using BucketType = typename Map::bucket_type;

  size_t m_size_{0};
  size_t min_freq_{0};
  std::vector<BucketType> buckets_;
  Map map_;  // key to {value, freq}
  boost::unordered_map<Key, typename std::list<Key>::iterator, Hash, Equal>
      m_iter_;  // key to list iterator;
  boost::unordered_map<size_t, std::list<Key>> m_freq_;  // freq to key list;
};

template <typename Key, typename Value, typename Hash, typename Equal>
void LfuBase<Key, Value, Hash, Equal>::Clear() {
  map_.clear();
  m_iter_.clear();
  m_freq_.clear();
  m_size_ = 0;
  min_freq_ = 0;
}

template <typename Key, typename Value, typename Hash, typename Equal>
Value& LfuBase<Key, Value, Hash, Equal>::Add(const Key& key, Value value) {
  if (map_.size() < buckets_.size()) {
    auto node =
        std::make_unique<LfuNode<Key, Value>>(Key{key}, std::move(value));
    return InsertNodeUpdateCounter(std::move(node)).GetValue();
  }
  Key key_1 = m_freq_[min_freq_].front();
  map_.erase(map_.find(key_1, map_.hash_function(), map_.key_eq()));
  m_iter_.erase(m_freq_[min_freq_].front());
  m_freq_[min_freq_].pop_front();
  --m_size_;
  auto node = std::make_unique<LfuNode<Key, Value>>(Key{key}, std::move(value));
  return InsertNodeUpdateCounter(std::move(node)).GetValue();
}

template <typename Key, typename Value, typename Hash, typename Equal>
LfuNode<Key, Value>& LfuBase<Key, Value, Hash, Equal>::InsertNodeUpdateCounter(
    std::unique_ptr<LfuNode<Key, Value>>&& node) noexcept {
  auto [it, ok] = map_.insert(*node);
  m_freq_[1].push_back(node->GetKey());
  m_iter_[node->GetKey()] = --m_freq_[1].end();
  min_freq_ = 1;
  ++m_size_;
  return *node.release();
}

template <typename Key, typename Value, typename Hash, typename Equal>
bool LfuBase<Key, Value, Hash, Equal>::Put(const Key& key, Value value) {
  auto it = map_.find(key, map_.hash_function(), map_.key_eq());
  if (it != map_.end()) {
    it->SetValue(std::move(value));
    size_t new_freq = it->Increase();

    m_freq_[new_freq - 1].erase(m_iter_[key]);
    m_freq_[new_freq].push_back(key);
    m_iter_[key] = --m_freq_[new_freq].end();
    if (m_freq_[min_freq_].size() == 0) {
      ++min_freq_;
    }
    return false;
  }

  Add(key, std::move(value));
  return true;
}

template <typename Key, typename Value, typename Hash, typename Equal>
template <typename... Args>
Value* LfuBase<Key, Value, Hash, Equal>::Emplace(const Key& key,
                                                 Args&&... args) {
  return Put(key, Value(std::forward<Args>(args)...));
}

template <typename Key, typename Value, typename Hash, typename Equal>
Value* LfuBase<Key, Value, Hash, Equal>::Get(const Key& key) {
  auto it = map_.find(key, map_.hash_function(), map_.key_eq());
  if (it == map_.end()) {
    return nullptr;
  }
  size_t new_freq = it->Increase();

  m_freq_[new_freq - 1].erase(m_iter_[key]);
  m_freq_[new_freq].push_back(key);
  m_iter_[key] = --m_freq_[new_freq].end();
  if (m_freq_[min_freq_].size() == 0) {
    ++min_freq_;
  }
  return &it->GetValue();
}

template <typename Key, typename Value, typename Hash, typename Equal>
void LfuBase<Key, Value, Hash, Equal>::SetMaxSize(size_t new_max_size) {
  std::vector<BucketType> new_buckets(new_max_size);
  map_.rehash(BucketTraits(new_buckets.data(), new_max_size));
  buckets_.swap(new_buckets);
}

template <typename Key, typename Value, typename Hash, typename Equal>
size_t LfuBase<Key, Value, Hash, Equal>::GetSize() const {
  return m_size_;
}
template <typename Key, typename Value, typename Hash, typename Equal>
LfuBase<Key, Value, Hash, Equal>::LfuBase(size_t max_size, const Hash& h,
                                          const Equal& e)
    : buckets_(max_size ? max_size : 1),
      map_(BucketTraits(buckets_.data(), buckets_.size()), h, e) {}
