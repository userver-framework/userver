#pragma once

/// @file rcu/rcu_map.hpp
/// @brief @copybrief rcu::RcuMap

#include <iterator>
#include <memory>
#include <unordered_map>
#include <utility>

#include <rcu/rcu.hpp>
#include <utils/traceful_exception.hpp>

namespace rcu {

/// Thrown on missing element access
class MissingKeyException : public utils::TracefulException {
 public:
  using utils::TracefulException::TracefulException;
};

/// Map-like structure allowing RCU keyset updates.
///
/// Only keyset changes are thread-safe in scope of this class.
/// Values are stored in `shared_ptr`s and are not copied during keyset change.
/// @note No synchronization is provided for value access, it must be
/// implemented by Value when necessary.
template <typename Key, typename Value>
class RcuMap final {
 public:
  template <typename ValueType>
  class IteratorImpl;

  using ValuePtr = std::shared_ptr<Value>;
  using Iterator = IteratorImpl<ValuePtr>;
  using ConstValuePtr = std::shared_ptr<const Value>;
  using ConstIterator = IteratorImpl<ConstValuePtr>;
  using Snapshot = std::unordered_map<Key, ConstValuePtr>;

  RcuMap() = default;

  RcuMap(const RcuMap&) = delete;
  RcuMap(RcuMap&&) = delete;
  RcuMap& operator=(const RcuMap&) = delete;
  RcuMap& operator=(RcuMap&&) = delete;

  /// Returns an estimated size of the map at some point in time
  size_t SizeApprox() const;

  /// @name Iteration support
  /// @details Keyset is fixed at the start of the iteration and is not affected
  /// by concurrent changes.
  /// @{
  ConstIterator begin() const;
  ConstIterator end() const;
  Iterator begin();
  Iterator end();
  /// @}

  /// @brief Returns a readonly value pointer by its key if exists
  /// @throws MissingKeyException if the key is not present
  const ConstValuePtr operator[](const Key&) const;

  /// @brief Returns a modifiable value pointer by key if exists or
  /// default-creates one
  const ValuePtr operator[](const Key&);

  // TODO: add multiple keys in one txn?

  /// @brief Returns a readonly value pointer by its key or an empty pointer
  const ConstValuePtr Get(const Key&) const;

  /// @brief Returns a modifiable value pointer by key or an empty pointer
  const ValuePtr Get(const Key&);

  /// @brief Removes a key from the map
  /// @returns whether the key was present
  bool Erase(const Key&);

  /// @brief Removes a key from the map returning its value
  /// @returs a value if the key was present, empty pointer otherwise
  ValuePtr Pop(const Key&);

  /// Resets the map to an empty state
  void Clear();

  /// @brief Returns a readonly copy of the map
  /// @note Equivalent to `{begin(), end()}` construct, preferable
  /// for long-running operations.
  Snapshot GetSnapshot() const;

 private:
  using MapType = std::unordered_map<Key, ValuePtr>;

  rcu::Variable<MapType> rcu_;
};

template <typename K, typename V>
template <typename ValueType>
class RcuMap<K, V>::IteratorImpl final {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = ptrdiff_t;
  using value_type = std::pair<K, ValueType>;
  using reference = const value_type&;
  using pointer = const value_type*;

  IteratorImpl() = default;
  explicit IteratorImpl(const Variable<MapType>&);

  IteratorImpl operator++(int);
  IteratorImpl& operator++();
  reference operator*() const;
  pointer operator->() const;

  bool operator==(const IteratorImpl&) const;
  bool operator!=(const IteratorImpl&) const;

 private:
  void UpdateCurrent();

  std::shared_ptr<ReadablePtr<MapType>> ptr_;
  typename MapType::const_iterator it_;
  value_type current_;
};

template <typename K, typename V>
typename RcuMap<K, V>::ConstIterator RcuMap<K, V>::begin() const {
  return IteratorImpl<ConstValuePtr>(rcu_);
}

template <typename K, typename V>
typename RcuMap<K, V>::ConstIterator RcuMap<K, V>::end() const {
  return {};
}

template <typename K, typename V>
size_t RcuMap<K, V>::SizeApprox() const {
  auto ptr = rcu_.Read();
  return ptr->size();
}

template <typename K, typename V>
const typename RcuMap<K, V>::ConstValuePtr RcuMap<K, V>::operator[](
    const K& key) const {
  if (auto value = Get(key)) {
    return value;
  }
  throw MissingKeyException("Key ") << key << " is missing";
}

template <typename K, typename V>
const typename RcuMap<K, V>::ConstValuePtr RcuMap<K, V>::Get(
    const K& key) const {
  return const_cast<RcuMap<K, V>*>(this)->Get(key);
}

template <typename K, typename V>
typename RcuMap<K, V>::Iterator RcuMap<K, V>::begin() {
  return IteratorImpl<ValuePtr>(rcu_);
}

template <typename K, typename V>
typename RcuMap<K, V>::Iterator RcuMap<K, V>::end() {
  return {};
}

template <typename K, typename V>
const typename RcuMap<K, V>::ValuePtr RcuMap<K, V>::operator[](const K& key) {
  auto value = Get(key);
  if (!value) {
    auto txn = rcu_.StartWrite();
    auto insertion_result = txn->emplace(key, std::make_shared<V>());
    value = insertion_result.first->second;
    if (insertion_result.second) txn.Commit();
  }
  return value;
}

template <typename K, typename V>
const typename RcuMap<K, V>::ValuePtr RcuMap<K, V>::Get(const K& key) {
  auto snapshot = rcu_.Read();
  auto it = snapshot->find(key);
  if (it == snapshot->end()) return {};
  return it->second;
}

template <typename K, typename V>
bool RcuMap<K, V>::Erase(const K& key) {
  if (Get(key)) {
    auto txn = rcu_.StartWrite();
    if (txn->erase(key)) {
      txn.Commit();
      return true;
    }
  }
  return false;
}

template <typename K, typename V>
typename RcuMap<K, V>::ValuePtr RcuMap<K, V>::Pop(const K& key) {
  auto value = Get(key);
  if (value) {
    auto txn = rcu_.StartWrite();
    if (txn->erase(key)) txn.Commit();
  }
  return value;
}

template <typename K, typename V>
void RcuMap<K, V>::Clear() {
  rcu_.Assign({});
}

template <typename K, typename V>
typename RcuMap<K, V>::Snapshot RcuMap<K, V>::GetSnapshot() const {
  return {begin(), end()};
}

template <typename K, typename V>
template <typename ValueType>
RcuMap<K, V>::IteratorImpl<ValueType>::IteratorImpl(
    const Variable<MapType>& rcu)
    : ptr_(std::make_shared<ReadablePtr<MapType>>(rcu.Read())),
      it_((*ptr_)->begin()) {
  UpdateCurrent();
}

template <typename K, typename V>
template <typename ValueType>
typename RcuMap<K, V>::template IteratorImpl<ValueType>
RcuMap<K, V>::IteratorImpl<ValueType>::operator++(int) {
  IteratorImpl tmp(*this);
  ++*this;
  return tmp;
}

template <typename K, typename V>
template <typename ValueType>
typename RcuMap<K, V>::template IteratorImpl<ValueType>&
RcuMap<K, V>::IteratorImpl<ValueType>::operator++() {
  ++it_;
  UpdateCurrent();
  return *this;
}

template <typename K, typename V>
template <typename ValueType>
typename RcuMap<K, V>::template IteratorImpl<ValueType>::reference
    RcuMap<K, V>::IteratorImpl<ValueType>::operator*() const {
  return current_;
}

template <typename K, typename V>
template <typename ValueType>
typename RcuMap<K, V>::template IteratorImpl<ValueType>::pointer
    RcuMap<K, V>::IteratorImpl<ValueType>::operator->() const {
  return &current_;
}

template <typename K, typename V>
template <typename ValueType>
bool RcuMap<K, V>::template IteratorImpl<ValueType>::operator==(
    const IteratorImpl& rhs) const {
  if (ptr_) {
    if (rhs.ptr_) {
      return it_ == rhs.it_;
    } else {
      return it_ == (*ptr_)->end();
    }
  } else {
    return !rhs.ptr_ || rhs.it_ == (*rhs.ptr_)->end();
  }
}

template <typename K, typename V>
template <typename ValueType>
bool RcuMap<K, V>::template IteratorImpl<ValueType>::operator!=(
    const IteratorImpl& rhs) const {
  return !(*this == rhs);
}

template <typename K, typename V>
template <typename ValueType>
void RcuMap<K, V>::template IteratorImpl<ValueType>::UpdateCurrent() {
  if (ptr_ && it_ != (*ptr_)->end()) {
    current_ = *it_;
  }
}

}  // namespace rcu
