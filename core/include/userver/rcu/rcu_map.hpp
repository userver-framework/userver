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
#include <userver/utils/not_null.hpp>
#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace rcu {

namespace impl {

template <typename RcuMapTraits>
struct RcuTraitsFromRcuMapTraits : public DefaultRcuTraits {
    using MutexType = typename RcuMapTraits::MutexType;
    using DeleterType = typename RcuMapTraits::DeleterType;
};

struct ShouldInheritFromDefaultRcuMapTraits {};

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
template <typename Key>
struct DefaultRcuMapTraits : public impl::ShouldInheritFromDefaultRcuMapTraits {
    using Hash = std::hash<Key>;
    using KeyEqual = std::equal_to<Key>;
    using MutexType = engine::Mutex;
    using DeleterType = AsyncDeleter;
};

/// @brief Forward iterator for the rcu::RcuMap
///
/// Use member functions of rcu::RcuMap to retrieve the iterator.
template <typename Key, typename Value, typename IterValue, typename RcuMapTraits>
class RcuMapIterator final {
    static_assert(
        std::is_base_of_v<impl::ShouldInheritFromDefaultRcuMapTraits, RcuMapTraits>,
        "RcuMapTraits should inherit from rcu::DefaultRcuMapTraits"
    );
    using Hash = typename RcuMapTraits::Hash;
    using KeyEqual = typename RcuMapTraits::KeyEqual;
    using MapType = std::unordered_map<Key, std::shared_ptr<Value>, Hash, KeyEqual>;
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
/// Only keyset changes are thread-safe in scope of this class. Values are stored in `std::shared_ptr`s and are not
/// copied during keyset change. The map itself is implemented as @ref rcu::Variable, so every keyset change (e.g.
/// insert or erase) triggers the whole map copying.
///
/// @warning Inserting N elements one by one requires O(N^2) operations because each insertion copies the whole map.
///
/// Writer access is protected by `RcuMapTraits::MutexType`. The default @ref rcu::DefaultRcuMapTraits selects
/// @ref engine::Mutex. With it and other regular mutex types, concurrent keyset changes are serialized for the whole
/// map. Read-modify-write operations first acquire the mutex and then copy the latest committed map snapshot, so they
/// include changes made by preceding writers. The mutex is held until the new snapshot is committed or discarded,
/// while readers continue using an older snapshot without waiting. This guarantee concerns copying the map snapshot;
/// preparation of a candidate Value is documented by each insertion method separately.
///
/// @note No synchronization is provided for value access, it must be implemented by Value when necessary.
///
/// ## Example usage:
///
/// @snippet core/src/rcu/rcu_map_test.cpp  Sample rcu::RcuMap usage
///
/// @see @ref scripts/docs/en/userver/synchronization.md
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
    std::size_t SizeApprox() const;

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
    const utils::NotNull<ConstValuePtr> operator[](const Key&) const;

    /// @brief Returns a modifiable value pointer by key if exists or default-creates one
    /// @note Copies the whole map if the key doesn't exist.
    /// @note The decisive presence check and insertion are serialized with other writers. Concurrent calls for the
    /// same missing key don't overwrite each other and return pointers to the same published value, unless another
    /// writer removes or replaces the key in between.
    const utils::NotNull<ValuePtr> operator[](const Key&);

    /// @brief Inserts a new element into the container if there is no element with the key in the container.
    /// Returns a pair consisting of a pointer to the inserted element, or the
    /// already-existing element if no insertion happened, and a bool denoting whether the insertion took place.
    /// @note Copies the whole map if the key doesn't exist.
    /// @note The supplied Value pointer may be discarded if another writer inserts an equivalent key before this
    /// operation acquires the writer mutex.
    /// @note The decisive presence check and insertion are serialized with other writers. Concurrent calls for
    /// equivalent keys don't overwrite each other: at most one can return `inserted == true` while the key remains
    /// present.
    InsertReturnType Insert(const Key& key, ValuePtr value);

    /// @brief Inserts a new element into the container constructed in-place with
    /// the given args if there is no element with the key in the container.
    /// Returns a pair consisting of a pointer to the inserted element, or the
    /// already-existing element if no insertion happened, and a bool denoting whether the insertion took place.
    /// @note Copies the whole map if the key doesn't exist.
    /// @note The Value candidate is constructed before acquiring the writer mutex and may be discarded if another
    /// writer inserts an equivalent key first. @ref rcu::RcuMap::TryEmplace avoids this extra construction.
    /// @note The decisive presence check and insertion are serialized with other writers. Concurrent calls for
    /// equivalent keys don't overwrite each other: at most one can return `inserted == true` while the key remains
    /// present.
    template <typename... Args>
    InsertReturnType Emplace(const Key& key, Args&&... args);

    /// @brief If a key equivalent to `key` already exists in the container, does nothing. Otherwise, constructs a
    /// Value from the given args and inserts it into the map.
    /// Returns a pair consisting of a pointer to the inserted element, or the
    /// already-existing element if no insertion happened, and a bool denoting whether the insertion took place.
    /// @note After acquiring the writer mutex, the final presence check uses the latest committed map snapshot. Value
    /// construction happens only if this check succeeds, although function arguments are evaluated before the call.
    /// For concurrent calls with equivalent keys, at most one call can commit the insertion and return
    /// `inserted == true`; once it commits, the other calls return its value with `inserted == false`, unless another
    /// writer removes or replaces the key.
    template <typename... Args>
    InsertReturnType TryEmplace(const Key& key, Args&&... args);

    /// @brief If a key equivalent to `key` already exists in the container,
    /// replaces the associated value. Otherwise, inserts a new pair into the map.
    /// @note Serialized with other write operations by the writer mutex. Concurrent assignments are applied one by
    /// one; the last committed assignment determines the value.
    template <typename RawKey>
    void InsertOrAssign(RawKey&& key, ValuePtr value);

    /// @brief Returns a readonly value pointer by its key; nullptr (a default constructed ConstValuePtr) if no such key
    const ConstValuePtr Get(const Key& key) const;

    /// @brief Returns a modifiable value pointer by key; nullptr (a default constructed ValuePtr) if no such key
    const ValuePtr Get(const Key& key);

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
    /// @details Acquires the same writer mutex as all other map writes, then copies the latest committed map. The
    /// returned transaction owns the mutex until `Commit` or destruction, so concurrent write transactions proceed
    /// one by one. Readers don't wait for the transaction. Don't forget to `Commit` to apply the changes.
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
typename RcuMap<K, V, RcuMapTraits>::ConstIterator RcuMap<K, V, RcuMapTraits>::begin() const {
    auto ptr = rcu_.Read();
    const auto iter = ptr->cbegin();
    return typename RcuMap<K, V, RcuMapTraits>::ConstIterator(std::move(ptr), iter);
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::ConstIterator RcuMap<K, V, RcuMapTraits>::end() const {
    // End iterator must be empty, because otherwise begin and end calls will
    // return iterators that point into different map snapshots.
    return {};
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::Iterator RcuMap<K, V, RcuMapTraits>::begin() {
    auto ptr = rcu_.Read();
    const auto iter = ptr->cbegin();
    return {std::move(ptr), iter};
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::Iterator RcuMap<K, V, RcuMapTraits>::end() {
    // End iterator must be empty, because otherwise begin and end calls will
    // return iterators that point into different map snapshots.
    return {};
}

template <typename K, typename V, typename RcuMapTraits>
std::size_t RcuMap<K, V, RcuMapTraits>::SizeApprox() const {
    auto ptr = rcu_.Read();
    return ptr->size();
}

template <typename K, typename V, typename RcuMapTraits>
// Protects from assignment to map[key]
// NOLINTNEXTLINE(readability-const-return-type)
const utils::NotNull<typename RcuMap<K, V, RcuMapTraits>::ConstValuePtr> RcuMap<
    K,
    V,
    RcuMapTraits>::operator[](const K& key) const {
    if (auto value = Get(key)) {
        return utils::NotNull{value};
    }
    throw MissingKeyException("Key ") << key << " is missing";
}

template <typename K, typename V, typename RcuMapTraits>
// Protects from assignment to map[key]
// NOLINTNEXTLINE(readability-const-return-type)
const typename RcuMap<K, V, RcuMapTraits>::ConstValuePtr RcuMap<K, V, RcuMapTraits>::Get(const K& key) const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<RcuMap<K, V, RcuMapTraits>*>(this)->Get(key);
}

template <typename K, typename V, typename RcuMapTraits>
// Protects from assignment to map[key]
// NOLINTNEXTLINE(readability-const-return-type)
const utils::NotNull<typename RcuMap<K, V, RcuMapTraits>::ValuePtr> RcuMap<K, V, RcuMapTraits>::operator[](const K& key
) {
    auto value = Get(key);
    if (!value) {
        auto ptr = std::make_shared<V>();
        auto txn = rcu_.StartWrite();
        auto insertion_result = txn->emplace(key, std::move(ptr));
        value = insertion_result.first->second;
        if (insertion_result.second) {
            txn.Commit();
        }
    }
    return utils::NotNull{value};
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::InsertReturnType RcuMap<
    K,
    V,
    RcuMapTraits>::Insert(const K& key, typename RcuMap<K, V, RcuMapTraits>::ValuePtr value) {
    InsertReturnType result{.value = Get(key), .inserted = false};
    if (result.value) {
        return result;
    }

    return DoInsert(key, std::move(value));
}

template <typename K, typename V, typename RcuMapTraits>
template <typename... Args>
typename RcuMap<K, V, RcuMapTraits>::InsertReturnType RcuMap<
    K,
    V,
    RcuMapTraits>::Emplace(const K& key, Args&&... args) {
    InsertReturnType result{.value = Get(key), .inserted = false};
    if (result.value) {
        return result;
    }

    return DoInsert(key, std::make_shared<V>(std::forward<Args>(args)...));
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::InsertReturnType RcuMap<
    K,
    V,
    RcuMapTraits>::DoInsert(const K& key, typename RcuMap<K, V, RcuMapTraits>::ValuePtr value) {
    auto txn = rcu_.StartWrite();
    auto insertion_result = txn->emplace(key, std::move(value));
    InsertReturnType result{.value = insertion_result.first->second, .inserted = insertion_result.second};
    if (result.inserted) {
        txn.Commit();
    }
    return result;
}

template <typename K, typename V, typename RcuMapTraits>
template <typename... Args>
typename RcuMap<K, V, RcuMapTraits>::InsertReturnType RcuMap<
    K,
    V,
    RcuMapTraits>::TryEmplace(const K& key, Args&&... args) {
    InsertReturnType result{.value = Get(key), .inserted = false};
    if (!result.value) {
        auto txn = rcu_.StartWrite();
        auto insertion_result = txn->try_emplace(key, nullptr);
        if (insertion_result.second) {
            result.value = insertion_result.first->second = std::make_shared<V>(std::forward<Args>(args)...);
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
void RcuMap<Key, Value, RcuMapTraits>::InsertOrAssign(RawKey&& key, RcuMap::ValuePtr value) {
    auto txn = rcu_.StartWrite();
    txn->insert_or_assign(std::forward<RawKey>(key), std::move(value));
    txn.Commit();
}

template <typename K, typename V, typename RcuMapTraits>
// Protects from assignment to map[key]
// NOLINTNEXTLINE(readability-const-return-type)
const typename RcuMap<K, V, RcuMapTraits>::ValuePtr RcuMap<K, V, RcuMapTraits>::Get(const K& key) {
    auto snapshot = rcu_.Read();
    auto it = snapshot->find(key);
    if (it == snapshot->end()) {
        return {};
    }
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
typename RcuMap<K, V, RcuMapTraits>::ValuePtr RcuMap<K, V, RcuMapTraits>::Pop(const K& key) {
    auto value = Get(key);
    if (value) {
        auto txn = rcu_.StartWrite();
        if (txn->erase(key)) {
            txn.Commit();
        }
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
auto RcuMap<K, V, RcuMapTraits>::StartWrite() -> rcu::WritablePtr<RawMap, RcuTraits> {
    return rcu_.StartWrite();
}

template <typename K, typename V, typename RcuMapTraits>
typename RcuMap<K, V, RcuMapTraits>::Snapshot RcuMap<K, V, RcuMapTraits>::GetSnapshot() const {
    return {begin(), end()};
}

template <typename Key, typename Value, typename IterValue, typename RcuMapTraits>
RcuMapIterator<
    Key,
    Value,
    IterValue,
    RcuMapTraits>::RcuMapIterator(ReadablePtr<MapType, RcuTraits>&& ptr, typename MapType::const_iterator iter)
    : ptr_(std::move(ptr)),
      it_(iter)
{
    UpdateCurrent();
}

template <typename Key, typename Value, typename IterValue, typename RcuMapTraits>
auto RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator++(int) -> RcuMapIterator {
    RcuMapIterator tmp(*this);
    ++*this;
    return tmp;
}

template <typename Key, typename Value, typename IterValue, typename RcuMapTraits>
auto RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator++() -> RcuMapIterator& {
    ++it_;
    UpdateCurrent();
    return *this;
}

template <typename Key, typename Value, typename IterValue, typename RcuMapTraits>
auto RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator*() const -> reference {
    return current_;
}

template <typename Key, typename Value, typename IterValue, typename RcuMapTraits>
auto RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator->() const -> pointer {
    return &current_;
}

template <typename Key, typename Value, typename IterValue, typename RcuMapTraits>
bool RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator==(const RcuMapIterator& rhs) const {
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

template <typename Key, typename Value, typename IterValue, typename RcuMapTraits>
bool RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::operator!=(const RcuMapIterator& rhs) const {
    return !(*this == rhs);
}

template <typename Key, typename Value, typename IterValue, typename RcuMapTraits>
void RcuMapIterator<Key, Value, IterValue, RcuMapTraits>::UpdateCurrent() {
    if (it_ != (*ptr_)->end()) {
        current_ = *it_;
    }
}

}  // namespace rcu

USERVER_NAMESPACE_END
