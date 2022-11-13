#pragma once

#include <unordered_map>
#include <tuple>
#include <list>
#include <boost/unordered_map.hpp>

template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
class LfuBase final {
 public:
  explicit LfuBase(size_t max_size, const Hash &hash = Hash(), const Equal &equal = Equal());
  LfuBase(LfuBase &&other) noexcept : m_capacity_(std::move(other.m_capacity_)),
                                                 m_size_(std::move(other.m_size_)),
                                                 min_freq_(std::move(other.min_freq_)) {
      other.Clear();
  }
  LfuBase &operator=(LfuBase &&other) noexcept {
      if (this != &other) {
          Clear();
      }
  }
  void Clear();
  bool Put(const Key &key, Value value);
  bool Has(const Key &key);
  template<typename... Args>
  Value *Emplace(const Key &key, Args &&... args);

//  void Erase(const Key& key);
  Value *Get(const Key &key);
  void SetMaxSize(size_t new_max_size);

  size_t GetSize() const;

  LfuBase(const LfuBase &) = delete;
  LfuBase &operator=(const LfuBase &) = delete;

 private:
  size_t m_capacity_;
  size_t m_size_{0};
  size_t min_freq_{0};

  boost::unordered_map<Key, std::pair<Value, size_t>, Hash, Equal> m_map_; //key to {value,freq};
  boost::unordered_map<Key, typename std::list<Key>::iterator, Hash, Equal> m_iter_; //key to list iterator;
  boost::unordered_map<size_t, std::list<Key>> m_freq_;  //freq to key list;
};

template<typename Key, typename Value, typename Hash, typename Equal>
void LfuBase<Key, Value, Hash, Equal>::Clear() {
    m_map_.clear();
    m_iter_.clear();
    m_freq_.clear();
    m_size_ = 0;
    min_freq_ = 0;
}

template<typename Key, typename Value, typename Hash, typename Equal>
bool LfuBase<Key, Value, Hash, Equal>::Put(const Key &key, Value value) {
    Value *storedValue = Get(key);
    if (storedValue) {
        m_map_[key].first = value;
        return false;
    }

    if (m_size_ >= m_capacity_) {
        m_map_.erase(m_freq_[min_freq_].front());
        m_iter_.erase(m_freq_[min_freq_].front());
        m_freq_[min_freq_].pop_front();
        --m_size_;
    }

    m_map_[key] = {value, 1};
    m_freq_[1].push_back(key);
    m_iter_[key] = --m_freq_[1].end();
    min_freq_ = 1;
    ++m_size_;
    return true;
}

template<typename Key, typename Value, typename Hash, typename Equal>
template<typename... Args>
Value *LfuBase<Key, Value, Hash, Equal>::Emplace(const Key &key, Args &&... args) {
    return Put(key, Value(std::forward<Args>(args)...));
}

template<typename Key, typename Value, typename Hash, typename Equal>
Value *LfuBase<Key, Value, Hash, Equal>::Get(const Key &key) {
    if (m_map_.count(key) == 0) {
        return nullptr;
    }
    m_freq_[m_map_[key].second].erase(m_iter_[key]);
    ++m_map_[key].second;
    m_freq_[m_map_[key].second].push_back(key);
    m_iter_[key] = --m_freq_[m_map_[key].second].end();
    if (m_freq_[min_freq_].size() == 0) {
        ++min_freq_;
    }
    return &m_map_[key].first;
}

template<typename Key, typename Value, typename Hash, typename Equal>
void LfuBase<Key, Value, Hash, Equal>::SetMaxSize(size_t new_max_size) {
    m_capacity_ = new_max_size;
}

template<typename Key, typename Value, typename Hash, typename Equal>
size_t LfuBase<Key, Value, Hash, Equal>::GetSize() const {
    return m_size_;
}
template<typename Key, typename Value, typename Hash, typename Equal>
LfuBase<Key, Value, Hash, Equal>::LfuBase(size_t max_size, const Hash &h, const Equal &e) : m_capacity_(max_size) {
}
