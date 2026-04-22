#pragma once

/// @file userver/concurrent/impl/monotonic_concurrent_set.hpp
/// @brief @copybrief concurrent::impl::MonotonicConcurrentSet

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>  // for std::lock_guard
#include <utility>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/fused_allocations.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/optional_ref.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

namespace monotonic_concurrent_set {

// Item node for intrusive linked list in each bucket
template <typename T>
union ItemWrapper {
    T item;

    ItemWrapper() {}
    ~ItemWrapper() {}
};

// Item node for intrusive linked list in each bucket.
// Items themselves are stored separately to allow multiple tables to point to the same item without copies.
template <typename T>
struct ItemNode {
    T* item;
    ItemNode* next;
};

inline constexpr std::uintptr_t kLockBit = 1;
inline constexpr std::uintptr_t kPtrMask = ~kLockBit;

inline bool IsPowerOf2(std::size_t value) noexcept { return value != 0 && (value & (value - 1)) == 0; }

inline bool IsBucketLocked(std::uintptr_t value) noexcept { return (value & kLockBit) != 0; }

template <typename T>
inline std::uintptr_t MakeBucketValue(ItemNode<T>* ptr, bool locked) noexcept {
    static_assert(alignof(ItemNode<T>) >= 2, "Lowest bit of ItemNode* must be free");
    return reinterpret_cast<std::uintptr_t>(ptr) | (locked ? monotonic_concurrent_set::kLockBit : 0);
}

// Bucket is an atomic stack with lowest bit used for locking.
template <typename T>
struct Bucket {
    std::atomic<std::uintptr_t> value{0};

    void lock() noexcept {
        std::uintptr_t expected = value.load(std::memory_order_relaxed);
        while (true) {
            // Wait if already locked
            while (IsBucketLocked(expected)) {
                value.wait(expected, std::memory_order_relaxed);
                expected = value.load(std::memory_order_relaxed);
            }

            // Try to acquire lock
            std::uintptr_t desired = expected | kLockBit;
            if (value.compare_exchange_weak(expected, desired, std::memory_order_acquire, std::memory_order_relaxed)) {
                return;
            }
        }
    }

    void unlock() noexcept {
        std::uintptr_t val = value.load(std::memory_order_relaxed);
        UASSERT(IsBucketLocked(val));
        value.store(val & kPtrMask, std::memory_order_release);
        value.notify_one();
    }

    ItemNode<T>* LoadHead(std::memory_order order) const noexcept {
        return reinterpret_cast<ItemNode<T>*>(value.load(order) & kPtrMask);
    }
};

template <typename T>
inline constinit Bucket<T> kNullBucket{};

template <typename T>
struct Table {
    utils::span<Bucket<T>> buckets;
    utils::span<ItemNode<T>> nodes;
    utils::span<ItemWrapper<T>> items;
    Table* next{nullptr};
    std::atomic<std::size_t> item_counter{0};

    constexpr Table()
        : buckets(&kNullBucket<T>, &kNullBucket<T> + 1)
    {}

    explicit Table(utils::span<Bucket<T>> buckets, utils::span<ItemNode<T>> nodes, utils::span<ItemWrapper<T>> items)
        : buckets(buckets),
          nodes(nodes),
          items(items)
    {}

    static utils::NotNull<Table*> AllocateFused(std::size_t node_capacity, std::size_t item_capacity) {
        const std::size_t bucket_count = node_capacity * 2;  // load_factor=0.5

        UASSERT(node_capacity > 0);
        UASSERT(item_capacity > 0);
        UINVARIANT(IsPowerOf2(bucket_count), "Bucket count must be a power of 2 for bitwise AND hashing");

        utils::span<Bucket<T>> buckets;
        utils::span<ItemNode<T>> nodes;
        utils::span<ItemWrapper<T>> items;

        const auto table = utils::impl::AllocateFused<Table>(
            utils::impl::FusedArray{bucket_count, buckets},
            utils::impl::FusedArray{node_capacity, nodes},
            utils::impl::FusedArray{item_capacity, items}
        );

        std::construct_at(&*table, buckets, nodes, items);
        std::uninitialized_value_construct_n(buckets.data(), buckets.size());
        std::uninitialized_default_construct_n(nodes.data(), nodes.size());
        std::uninitialized_default_construct_n(items.data(), items.size());
        return table;
    }

    static void DeallocateFused(utils::NotNull<Table*> table) noexcept {
        std::size_t item_count = table->item_counter.load(std::memory_order_relaxed);
        if (item_count > table->items.size()) {
            item_count = table->items.size();
        }

        for (const auto& item : table->items.subspan(0, item_count)) {
            std::destroy_at(&item.item);
        }

        std::destroy_n(table->items.data(), table->items.size());
        std::destroy_n(table->nodes.data(), table->nodes.size());
        std::destroy_n(table->buckets.data(), table->buckets.size());
        std::destroy_at(&*table);
        utils::impl::DeallocateFused(table);
    }

    Bucket<T>& GetBucket(std::size_t hash) noexcept {
        UASSERT(IsPowerOf2(buckets.size()));
        return buckets[hash & (buckets.size() - 1)];
    }
};

template <typename T>
inline constinit Table<T> kNullTable{};

}  // namespace monotonic_concurrent_set

/// @brief A thread-safe monotonic concurrent set with lock-free reads.
///
/// This container grows monotonically - items can only be added, never removed.
///
/// @warning Items are emplaced under `std::mutex`. Do not wait in item constructors!
///
/// @tparam T The value type
/// @tparam Hash Hash function object type
/// @tparam KeyEqual Equality comparison function object type
template <typename T, typename Hash = std::hash<T>, typename KeyEqual = std::equal_to<T>>
class MonotonicConcurrentSet final {
public:
    /// @brief Construct with initial capacity
    /// @param initial_capacity Initial table's capacity (default: 8)
    explicit MonotonicConcurrentSet(std::size_t initial_capacity = 8);

    ~MonotonicConcurrentSet();

    MonotonicConcurrentSet(const MonotonicConcurrentSet&) = delete;
    MonotonicConcurrentSet& operator=(const MonotonicConcurrentSet&) = delete;
    MonotonicConcurrentSet(MonotonicConcurrentSet&&) = delete;
    MonotonicConcurrentSet& operator=(MonotonicConcurrentSet&&) = delete;

    /// @brief Emplace an item into the set if there is no equal item already present.
    /// @param key The key to search for (used for equality comparison)
    /// @param args Arguments to construct T
    /// @return Pair of reference to the item and bool indicating if insertion took place
    template <typename Key, typename... Args>
    std::pair<T&, bool> TryEmplace(const Key& key, Args&&... args);

    /// @brief Find an item in the set. `Key` must be hashable by `Hash` and comparable by `KeyEqual`.
    /// @param key The key to search for
    /// @return Optional reference to the found item
    template <typename Key>
    utils::OptionalRef<const T> Find(const Key& key) const;

    /// @overload
    template <typename Key>
    utils::OptionalRef<T> Find(const Key& key);

    /// @brief Call the passed `visitor` on all contained items.
    /// @param visitor A callable that accepts `const T&`
    template <typename Visitor>
    requires std::invocable<Visitor&, const T&>
    void Visit(Visitor visitor) const;

    /// @brief Call the passed `visitor` on all contained items.
    /// @param visitor A callable that accepts `T&`
    template <typename Visitor>
    requires std::invocable<Visitor&, T&>
    void Visit(Visitor visitor);

private:
    using ItemWrapper = monotonic_concurrent_set::ItemWrapper<T>;
    using ItemNode = monotonic_concurrent_set::ItemNode<T>;
    using Bucket = monotonic_concurrent_set::Bucket<T>;
    using Table = monotonic_concurrent_set::Table<T>;

    ItemNode& GetNodeForItemIndex(Table& table, std::size_t item_index) const noexcept {
        // First few nodes of the current table are used for items from previous tables, see FillNewTable.
        const std::size_t node_index = (table.nodes.size() - table.items.size()) + item_index;
        return table.nodes[node_index];
    }

    template <typename Key>
    T* FindInBucket(ItemNode* head, const Key& key) const;

    template <typename Key>
    T* DoFind(const Key& key) const;

    template <typename ItemReference, typename Visitor>
    void DoVisit(Visitor visitor) const;

    template <typename Key, typename... Args>
    std::pair<T*, bool> TryEmplaceLocked(Table& table, Bucket& bucket, const Key& key, Args&&... args);

    void FillNewTable(Table& old_table, Table& new_table) noexcept;

    void GrowInitial();

    void Grow(Table& old_table);

    Hash hasher_;
    KeyEqual key_equal_;
    std::size_t initial_capacity_;
    std::atomic<utils::NotNull<Table*>> head_{monotonic_concurrent_set::kNullTable<T>};
    std::mutex grow_mutex_;
};

template <typename T, typename Hash, typename KeyEqual>
MonotonicConcurrentSet<T, Hash, KeyEqual>::MonotonicConcurrentSet(std::size_t initial_capacity)
    : hasher_(),
      key_equal_(),
      initial_capacity_(initial_capacity)
{
    UINVARIANT(monotonic_concurrent_set::IsPowerOf2(initial_capacity), "Capacity must be a power of 2");
}

template <typename T, typename Hash, typename KeyEqual>
MonotonicConcurrentSet<T, Hash, KeyEqual>::~MonotonicConcurrentSet() {
    utils::NotNull<Table*> current = head_.load(std::memory_order_relaxed);
    while (&*current != &monotonic_concurrent_set::kNullTable<T>) {
        Table* next_table = current->next;
        Table::DeallocateFused(current);
        current = utils::NotNull{next_table};
    }
}

template <typename T, typename Hash, typename KeyEqual>
template <typename Key>
T* MonotonicConcurrentSet<T, Hash, KeyEqual>::FindInBucket(ItemNode* head, const Key& key) const {
    ItemNode* node = head;
    while (node) {
        UASSERT(node->item != nullptr);
        if (key_equal_(*node->item, key)) {
            return node->item;
        }
        node = node->next;
    }
    return nullptr;
}

template <typename T, typename Hash, typename KeyEqual>
template <typename Key>
T* MonotonicConcurrentSet<T, Hash, KeyEqual>::DoFind(const Key& key) const {
    const std::size_t hash = hasher_(key);
    Table& table = *head_.load(std::memory_order_acquire);
    const auto& bucket = table.GetBucket(hash);
    return FindInBucket(bucket.LoadHead(std::memory_order_acquire), key);
}

template <typename T, typename Hash, typename KeyEqual>
template <typename Key>
utils::OptionalRef<const T> MonotonicConcurrentSet<T, Hash, KeyEqual>::Find(const Key& key) const {
    if (T* const found = DoFind(key)) {
        return utils::OptionalRef<const T>(*found);
    }
    return std::nullopt;
}

template <typename T, typename Hash, typename KeyEqual>
template <typename Key>
utils::OptionalRef<T> MonotonicConcurrentSet<T, Hash, KeyEqual>::Find(const Key& key) {
    if (T* found = DoFind(key)) {
        return utils::OptionalRef<T>(*found);
    }
    return std::nullopt;
}

template <typename T, typename Hash, typename KeyEqual>
template <typename ItemReference, typename Visitor>
void MonotonicConcurrentSet<T, Hash, KeyEqual>::DoVisit(Visitor visitor) const {
    auto table = head_.load(std::memory_order_acquire);
    for (const auto& bucket : table->buckets) {
        for (const ItemNode* node = bucket.LoadHead(std::memory_order_acquire); node != nullptr; node = node->next) {
            T* item = node->item;
            UASSERT(item);
            visitor(static_cast<ItemReference>(*item));
        }
    }
}

template <typename T, typename Hash, typename KeyEqual>
template <typename Visitor>
requires std::invocable<Visitor&, const T&>
void MonotonicConcurrentSet<T, Hash, KeyEqual>::Visit(Visitor visitor) const {
    DoVisit<const T&>(visitor);
}

template <typename T, typename Hash, typename KeyEqual>
template <typename Visitor>
requires std::invocable<Visitor&, T&>
void MonotonicConcurrentSet<T, Hash, KeyEqual>::Visit(Visitor visitor) {
    DoVisit<T&>(visitor);
}

template <typename T, typename Hash, typename KeyEqual>
template <typename Key, typename... Args>
std::pair<T*, bool> MonotonicConcurrentSet<
    T,
    Hash,
    KeyEqual>::TryEmplaceLocked(Table& table, Bucket& bucket, const Key& key, Args&&... args) {
    ItemNode* const bucket_head = bucket.LoadHead(std::memory_order_relaxed);

    if (T* const existing = FindInBucket(bucket_head, key)) {
        return {existing, false};
    }

    const std::size_t item_index = table.item_counter.fetch_add(1, std::memory_order_relaxed);

    if (item_index >= table.items.size()) {
        // Table is full, request rehashing.
        return {nullptr, false};
    }

    std::construct_at(&table.items[item_index].item, std::forward<Args>(args)...);
    T& new_item = table.items[item_index].item;

    ItemNode& new_node = GetNodeForItemIndex(table, item_index);
    new_node.item = &new_item;
    new_node.next = bucket_head;
    bucket.value.store(monotonic_concurrent_set::MakeBucketValue(&new_node, true), std::memory_order_release);

    return {&new_item, true};
}

template <typename T, typename Hash, typename KeyEqual>
void MonotonicConcurrentSet<T, Hash, KeyEqual>::FillNewTable(Table& old_table, Table& new_table) noexcept {
    ItemNode* next_new_node = new_table.nodes.data();

    for (Table* src_table = &old_table; src_table != &monotonic_concurrent_set::kNullTable<T>;
         src_table = src_table->next)
    {
        UASSERT(src_table != &new_table);
        UASSERT(src_table->item_counter.load(std::memory_order_relaxed) >= src_table->items.size());

        for (auto& item_wrapper : src_table->items) {
            T& item = item_wrapper.item;

            const std::size_t hash = hasher_(item);
            auto& bucket = new_table.GetBucket(hash);

            ItemNode& new_node = *(next_new_node++);
            new_node.item = &item;
            new_node.next = bucket.LoadHead(std::memory_order_relaxed);
            bucket.value.store(monotonic_concurrent_set::MakeBucketValue(&new_node, false), std::memory_order_relaxed);
        }
    }
}

template <typename T, typename Hash, typename KeyEqual>
void MonotonicConcurrentSet<T, Hash, KeyEqual>::GrowInitial() {
    std::lock_guard grow_lock(grow_mutex_);

    if (&*head_.load(std::memory_order_relaxed) != &monotonic_concurrent_set::kNullTable<T>) {
        return;
    }

    const auto table = Table::AllocateFused(initial_capacity_, initial_capacity_);
    table->next = &monotonic_concurrent_set::kNullTable<T>;
    head_.store(table, std::memory_order_release);
}

template <typename T, typename Hash, typename KeyEqual>
void MonotonicConcurrentSet<T, Hash, KeyEqual>::Grow(Table& old_table) {
    std::lock_guard grow_lock(grow_mutex_);

    if (&*head_.load(std::memory_order_acquire) != &old_table) {
        return;
    }

    // Node count grows as: x, 2x, 4x, 8x, ... - because each table accommodates all previous items.
    const std::size_t old_node_capacity = old_table.nodes.size();
    const std::size_t new_node_capacity = old_node_capacity * 2;

    // Item count grows as: x, x, 2x, 4x, ... - so that total item limit per bucket table has a growth factor of 2.
    const std::size_t old_item_capacity = old_table.items.size();
    const std::size_t new_item_capacity =
        (old_table.next == &monotonic_concurrent_set::kNullTable<T>) ? old_item_capacity : old_item_capacity * 2;

    auto new_table = Table::AllocateFused(new_node_capacity, new_item_capacity);

    for (auto& bucket : old_table.buckets) {
        bucket.lock();
    }

    FillNewTable(old_table, *new_table);

    new_table->next = &old_table;
    head_.store(new_table, std::memory_order_release);

    for (auto& bucket : old_table.buckets) {
        bucket.unlock();
    }
}

template <typename T, typename Hash, typename KeyEqual>
template <typename Key, typename... Args>
std::pair<T&, bool> MonotonicConcurrentSet<T, Hash, KeyEqual>::TryEmplace(const Key& key, Args&&... args) {
    const std::size_t hash = hasher_(key);

    while (true) {
        Table& current = *head_.load(std::memory_order_acquire);

        if (&current == &monotonic_concurrent_set::kNullTable<T>) {
            GrowInitial();
            continue;
        }

        auto& bucket = current.GetBucket(hash);

        // Fast path: lock-free search (same as Find) - avoid lock when item already exists
        if (T* const existing = FindInBucket(bucket.LoadHead(std::memory_order_acquire), key)) {
            return {*existing, false};
        }

        {
            std::lock_guard lock(bucket);

            // Table is only changed under all locks of the previous table. Because of acquire-release semantics
            // of locks, if the table has already changed, we are guaranteed to see it here.
            if (&*head_.load(std::memory_order_relaxed) != &current) {
                continue;
            }

            // Value of 'args' is only consumed if item construction happens.
            // NOLINTNEXTLINE(bugprone-use-after-move)
            auto [item, inserted] = TryEmplaceLocked(current, bucket, key, std::forward<Args>(args)...);

            if (item) {
                return {*item, inserted};
            }
        }

        Grow(current);
    }
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
