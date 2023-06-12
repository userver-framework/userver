#pragma once

/// @file userver/rcu/rcu_map.hpp
/// @brief @copybrief rcu::RcuMap

#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <userver/rcu/rcu.hpp>
#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace rcu {

namespace impl {
template <typename RcuMapTraits>
struct RcuTraitsFromRcuMapTraits {
  using MutexType = typename RcuMapTraits::MutexType;
};
}  // namespace impl

/// Thrown on missing element access
class MissingKeyException : public utils::TracefulException {
 public:
  using utils::TracefulException::TracefulException;
};

/// Default RcuMap traits.
/// Member types:
/// - `Hash` is a functor type that returns hash value for `Key`
/// - `keyEqual` is a functor type that provide equality test for two values of
/// type `Key`
/// - `MutexType` is a writer's mutex type that has to be used to protect
/// structure on update
template <typename Key, typename Value>
struct DefaultRcuMapTraits {
  using Hash = std::hash<Key>;
  using KeyEqual = std::equal_to<Key>;
  using MutexType = engine::Mutex;
};

/// @brief Forward iterator for the rcu::RcuMap
///
/// Use member functions of rcu::RcuMap to retrieve the iterator.
template <typename Key, typename Value, typename IterValue,
          typename RcuMapTraits>
class RcuMapIterator final {
  using Hash = typename RcuMapTraits::Hash;
  using KeyEqual = typename RcuMapTraits::KeyEqual;
  using MapType =
      std::unordered_map<Key, std::shared_ptr<Value>, Hash, KeyEqual>;
  using BaseIterator = typename MapType::const_iterator;
  using RcuTraits = typename impl::RcuTraitsFromRcuMapTraits<RcuMapTraits>;

 public:
  using iterator_category = std::input_iterator_tag;
  using difference_type = ptrdiff_t;
  using value_type = std::pair<Key, std::shared_ptr<IterValue>>;
  using reference = const value_type&;
  using pointer = const value_type*;

  RcuMapIterator() = default;

  RcuMapIterator operator++(int);
  RcuMapIterator& operator++();
  reference operator*() const;
  pointer operator->() const;

  bool operator==(const RcuMapIterator&) const;
  bool operator!=(const RcuMapIterator&) const;

  /// @cond
  /// For internal use only
  RcuMapIterator(ReadablePtr<MapType, RcuTraits>&& ptr, BaseIterator iter);
  /// @endcond

 private:
  void UpdateCurrent();

  std::optional<ReadablePtr<MapType, RcuTraits>> ptr_;
  BaseIterator it_;
  value_type current_;
};

/// @ingroup userver_concurrency userver_containers
///
/// @brief Map-like structure allowing RCU keyset updates.
///
/// Only keyset changes are thread-safe in scope of this class.
/// Values are stored in `shared_ptr`s and are not copied during keyset change.
/// The map itself is implemented as rcu::Variable, so every keyset change
/// (e.g. insert or erase) triggers the whole map copying.
/// @note No synchronization is provided for value access, it must be
/// implemented by Value when necessary.
///
/// ## Example usage:
///
/// @snippet rcu/rcu_map_test.cpp  Sample rcu::RcuMap usage
///
/// @see @ref md_en_userver_synchronization
template <typename Key, typename Value, typename RcuMapTraits>
class RcuMap final {
  using RcuTraits = typename impl::RcuTraitsFromRcuMapTraits<RcuMapTraits>;

 public:
  static_assert(!std::is_reference_v<Key>);
  static_assert(!std::is_reference_v<Value>);
  static_assert(!std::is_const_v<Key>);

  template <typename ValuePtrType>
  struct InsertReturnTypeImpl;

  using Hash = typename RcuMapTraits::Hash;
  using KeyEqual = typename RcuMapTraits::KeyEqual;
  using MutexType = typename RcuMapTraits::MutexType;
  using ValuePtr = std::shared_ptr<Value>;
  using Iterator = RcuMapIterator<Key, Value, Value, RcuMapTraits>;
  using ConstValuePtr = std::shared_ptr<const Value>;
  using ConstIterator = RcuMapIterator<Key, Value, const Value, RcuMapTraits>;
  using RawMap = std::unordered_map<Key, ValuePtr, Hash, KeyEqual>;
  using Snapshot = std::unordered_map<Key, ConstValuePtr, Hash, KeyEqual>;
  using InsertReturnType = InsertReturnTypeImpl<ValuePtr>;

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
  /// @note Copies the whole map if the key doesn't exist.
  const ValuePtr operator[](const Key&);

  /// @brief Inserts a new element into the container if there is no element
  /// with the key in the container.
  /// Returns a pair consisting of a pointer to the inserted element, or the
  /// already-existing element if no insertion happened, and a bool denoting
  /// whether the insertion took place.
  /// @note Copies the whole map if the key doesn't exist.
  InsertReturnType Insert(const Key& key, ValuePtr value);

  /// @brief Inserts a new element into the container constructed in-place with
  /// the given args if there is no element with the key in the container.
  /// Returns a pair consisting of a pointer to the inserted element, or the
  /// already-existing element if no insertion happened, and a bool denoting
  /// whether the insertion took place.
  /// @note Copies the whole map if the key doesn't exist.
  template <typename... Args>
  InsertReturnType Emplace(const Key& key, Args&&... args);

  /// @brief If a key equivalent to `key` already exists in the container, does
  /// nothing.
  /// Otherwise, behaves like `Emplace` except that the element is constructed
  /// as `std::make_shared<Value>(std::piecewise_construct,
  /// std::forward_as_tuple(key),
  /// std::forward_as_tuple(std::forward<Args>(args)...))`.
  /// Returns a pair consisting of a pointer to the inserted element, or the
  /// already-existing element if no insertion happened, and a bool denoting
  /// whether the insertion took place.
  template <typename... Args>
  InsertReturnType TryEmplace(const Key& key, Args&&... args);

  /// @brief If a key equivalent to `key` already exists in the container,
  /// replaces the associated value. Otherwise, inserts a new pair into the map.
  template <typename RawKey>
  void InsertOrAssign(RawKey&& key, ValuePtr value);

  /// @brief Returns a readonly value pointer by its key or an empty pointer
  const ConstValuePtr Get(const Key&) const;

  /// @brief Returns a modifiable value pointer by key or an empty pointer
  const ValuePtr Get(const Key&);

  /// @brief Removes a key from the map
  /// @returns whether the key was present
  /// @note Copies the whole map, might be slow for large maps.
  bool Erase(const Key&);

  /// @brief Removes a key from the map returning its value
  /// @returns a value if the key was present, empty pointer otherwise
  /// @note Copies the whole map, might be slow for large maps.
  ValuePtr Pop(const Key&);

  /// Resets the map to an empty state
  void Clear();

  /// Replace current data by data from `new_map`.
  void Assign(RawMap new_map);

  /// @brief Starts a transaction, used to perform a series of arbitrary changes
  /// to the map.
  /// @details The map is copied. Don't forget to `Commit` to apply the changes.
  rcu::WritablePtr<RawMap, RcuTraits> StartWrite();

  /// @brief Returns a readonly copy of the map
  /// @note Equivalent to `{begin(), end()}` construct, preferable
  /// for long-running operations.
  Snapshot GetSnapshot() const;

 private:
  InsertReturnType DoInsert(const Key& key, ValuePtr value);

  rcu::Variable<RawMap, RcuTraits> rcu_;
};

template <typename K, typename V, typename RcuMapTraits>
template <typename ValuePtrType>
struct RcuMap<K, V, RcuMapTraits>::InsertReturnTypeImpl {
  ValuePtrType value;
  bool inserted;
};

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::ConstIterator
RcuMap<K, V, RcuMapTraits>::begin() const {
  auto ptr = rcu_.Read();
  const auto iter = ptr->cbegin();
  return
      typename RcuMap<K, V, RcuMapTraits>::ConstIterator(std::move(ptr), iter);
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::ConstIterator
RcuMap<K, V, RcuMapTraits>::end() const {
  // End iterator must be empty, because otherwise begin and end calls will
  // return iterators that point into different map snapshots.
  return {};
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::Iterator
RcuMap<K, V, RcuMapTraits>::begin() {
  auto ptr = rcu_.Read();
  const auto iter = ptr->cbegin();
  return {std::move(ptr), iter};
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::Iterator
RcuMap<K, V, RcuMapTraits>::end() {
  // End iterator must be empty, because otherwise begin and end calls will
  // return iterators that point into different map snapshots.
  return {};
}

template <typename K, typename V, typename RcuMapTraits>
size_t RcuMap<K, V, RcuMapTraits>::SizeApprox() const {
  auto ptr = rcu_.Read();
  return ptr->size();
}

template <typename K, typename V, typename RcuMapTraits>
// Protects from assignment to map[key]
// NOLINTNEXTLINE(readability-const-return-type)
const typename RcuMap<K, V, RcuMapTraits>::ConstValuePtr
RcuMap<K, V, RcuMapTraits>::operator[](const K& key) const {
  if (auto value = Get(key)) {
    return value;
  }
  throw MissingKeyException("Key ") << key << " is missing";
}

template <typename K, typename V, typename RcuMapTraits>
// Protects from assignment to map[key]
// NOLINTNEXTLINE(readability-const-return-type)
const typename RcuMap<K, V, RcuMapTraits>::ConstValuePtr
RcuMap<K, V, RcuMapTraits>::Get(const K& key) const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<RcuMap<K, V, RcuMapTraits>*>(this)->Get(key);
}

template <typename K, typename V, typename RcuMapTraits>
// Protects from assignment to map[key]
// NOLINTNEXTLINE(readability-const-return-type)
const typename RcuMap<K, V, RcuMapTraits>::ValuePtr
RcuMap<K, V, RcuMapTraits>::operator[](const K& key) {
  auto value = Get(key);
  if (!value) {
    auto txn = rcu_.StartWrite();
    auto insertion_result = txn->emplace(key, std::make_shared<V>());
    value = insertion_result.first->second;
    if (insertion_result.second) txn.Commit();
  }
  return value;
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::InsertReturnType
RcuMap<K, V, RcuMapTraits>::Insert(
    const K& key, typename RcuMap<K, V, RcuMapTraits>::ValuePtr value) {
  InsertReturnType result{Get(key), false};
  if (result.value) return result;

  return DoInsert(key, std::move(value));
}

template <typename K, typename V, typename RcuMapTraits>
template <typename... Args>
typename RcuMap<K, V, RcuMapTraits>::InsertReturnType
RcuMap<K, V, RcuMapTraits>::Emplace(const K& key, Args&&... args) {
  InsertReturnType result{Get(key), false};
  if (result.value) return result;

  return DoInsert(key, std::make_shared<V>(std::forward<Args>(args)...));
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::InsertReturnType
RcuMap<K, V, RcuMapTraits>::DoInsert(
    const K& key, typename RcuMap<K, V, RcuMapTraits>::ValuePtr value) {
  auto txn = rcu_.StartWrite();
  auto insertion_result = txn->emplace(key, std::move(value));
  InsertReturnType result{insertion_result.first->second,
                          insertion_result.second};
  if (result.inserted) txn.Commit();
  return result;
}

template <typename K, typename V, typename RcuMapTraits>
template <typename... Args>
typename RcuMap<K, V, RcuMapTraits>::InsertReturnType
RcuMap<K, V, RcuMapTraits>::TryEmplace(const K& key, Args&&... args) {
  InsertReturnType result{Get(key), false};
  if (!result.value) {
    auto txn = rcu_.StartWrite();
    auto insertion_result = txn->try_emplace(key, nullptr);
    if (insertion_result.second) {
      result.value = insertion_result.first->second =
          std::make_shared<V>(std::forward<Args>(args)...);
      txn.Commit();
      result.inserted = true;
    } else {
      result.value = insertion_result.first->second;
    }
  }
  return result;
}

template <typename Key, typename Value, typename RcuMapTraits>
template <typename RawKey>
void RcuMap<Key, Value, RcuMapTraits>::InsertOrAssign(RawKey&& key,
                                                      RcuMap::ValuePtr value) {
  auto txn = rcu_.StartWrite();
  txn->insert_or_assign(std::forward<RawKey>(key), std::move(value));
  txn.Commit();
}

template <typename K, typename V, typename RcuMapTraits>
// Protects from assignment to map[key]
// NOLINTNEXTLINE(readability-const-return-type)
const typename RcuMap<K, V, RcuMapTraits>::ValuePtr
RcuMap<K, V, RcuMapTraits>::Get(const K& key) {
  auto snapshot = rcu_.Read();
  auto it = snapshot->find(key);
  if (it == snapshot->end()) return {};
  return it->second;
}

template <typename K, typename V, typename RcuMapTraits>
bool RcuMap<K, V, RcuMapTraits>::Erase(const K& key) {
  if (Get(key)) {
    auto txn = rcu_.StartWrite();
    if (txn->erase(key)) {
      txn.Commit();
      return true;
    }
  }
  return false;
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::ValuePtr RcuMap<K, V, RcuMapTraits>::Pop(
    const K& key) {
  auto value = Get(key);
  if (value) {
    auto txn = rcu_.StartWrite();
    if (txn->erase(key)) txn.Commit();
  }
  return value;
}

template <typename K, typename V, typename RcuMapTraits>
void RcuMap<K, V, RcuMapTraits>::Clear() {
  rcu_.Assign({});
}

template <typename K, typename V, typename RcuMapTraits>
void RcuMap<K, V, RcuMapTraits>::Assign(RawMap new_map) {
  rcu_.Assign(std::move(new_map));
}

template <typename K, typename V, typename RcuMapTraits>
auto RcuMap<K, V, RcuMapTraits>::StartWrite()
    -> rcu::WritablePtr<RawMap, RcuTraits> {
  return rcu_.StartWrite();
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::Snapshot
RcuMap<K, V, RcuMapTraits>::GetSnapshot() const {
  return {begin(), end()};
}

template <typename Key, typename Value, typename IterValue,
          typename RcuMapTraits>
RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::RcuMapIterator(
    ReadablePtr<MapType, RcuTraits>&& ptr,
    typename MapType::const_iterator iter)
    : ptr_(std::move(ptr)), it_(iter) {
  UpdateCurrent();
}

template <typename Key, typename Value, typename IterValue,
          typename RcuMapTraits>
auto RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator++(int)
    -> RcuMapIterator {
  RcuMapIterator tmp(*this);
  ++*this;
  return tmp;
}

template <typename Key, typename Value, typename IterValue,
          typename RcuMapTraits>
auto RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator++()
    -> RcuMapIterator& {
  ++it_;
  UpdateCurrent();
  return *this;
}

template <typename Key, typename Value, typename IterValue,
          typename RcuMapTraits>
auto RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator*() const
    -> reference {
  return current_;
}

template <typename Key, typename Value, typename IterValue,
          typename RcuMapTraits>
auto RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator->() const
    -> pointer {
  return &current_;
}

template <typename Key, typename Value, typename IterValue,
          typename RcuMapTraits>
bool RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator==(
    const RcuMapIterator& rhs) const {
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

template <typename Key, typename Value, typename IterValue,
          typename RcuMapTraits>
bool RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator!=(
    const RcuMapIterator& rhs) const {
  return !(*this == rhs);
}

template <typename Key, typename Value, typename IterValue,
          typename RcuMapTraits>
void RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::UpdateCurrent() {
  if (it_ != (*ptr_)->end()) {
    current_ = *it_;
  }
}

}  // namespace rcu

USERVER_NAMESPACE_END
