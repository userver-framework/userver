#pragma once

/// @file userver/utils/statistics/impl/monotonic_concurrent_statistics_map.hpp
/// @brief Thread-safe monotonic concurrent map with transparent hashing.

#include <cstddef>
#include <utility>

#include <userver/concurrent/impl/monotonic_concurrent_set.hpp>
#include <userver/utils/optional_ref.hpp>

#include <utils/statistics/impl/statistics_key.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

/// @brief Entry stored in the map: key-value pair.
template <typename T>
struct StatisticsMapEntry {
    StatisticsKey key;
    T value;

    template <typename... Args>
    explicit StatisticsMapEntry(StatisticsKey k, Args&&... args)
        : key(std::move(k)),
          value(std::forward<Args>(args)...)
    {}

    template <typename... Args>
    explicit StatisticsMapEntry(StatisticsKeyView k, Args&&... args)
        : key(k.Materialize()),
          value(std::forward<Args>(args)...)
    {}
};

/// @brief Hash for map entry - hashes only the key part.
template <typename T>
struct StatisticsMapEntryHash {
    using is_transparent [[maybe_unused]] = void;

    std::size_t operator()(const StatisticsMapEntry<T>& entry) const noexcept { return StatisticsKeyHash{}(entry.key); }
    std::size_t operator()(const StatisticsKey& key) const noexcept { return StatisticsKeyHash{}(key); }
    std::size_t operator()(const StatisticsKeyView& key) const noexcept { return StatisticsKeyHash{}(key); }
};

/// @brief Equality for map entry - compares only the key part.
template <typename T>
struct StatisticsMapEntryEqual {
    using is_transparent [[maybe_unused]] = void;

    bool operator()(const StatisticsMapEntry<T>& a, const StatisticsMapEntry<T>& b) const noexcept {
        return StatisticsKeyEqual{}(a.key, b.key);
    }
    bool operator()(const StatisticsMapEntry<T>& a, const StatisticsKey& b) const noexcept {
        return StatisticsKeyEqual{}(a.key, b);
    }
    bool operator()(const StatisticsKey& a, const StatisticsMapEntry<T>& b) const noexcept {
        return StatisticsKeyEqual{}(a, b.key);
    }
    bool operator()(const StatisticsMapEntry<T>& a, const StatisticsKeyView& b) const noexcept {
        return StatisticsKeyEqual{}(a.key, b);
    }
    bool operator()(const StatisticsKeyView& a, const StatisticsMapEntry<T>& b) const noexcept {
        return StatisticsKeyEqual{}(a, b.key);
    }
};

/// @brief Thread-safe monotonic concurrent map from (path, LabelsSpan) to T.
///
/// Uses transparent hashing - Find and TryEmplace accept StatisticsKeyView
/// without copying path or labels.
template <typename T>
class MonotonicConcurrentStatisticsMap final {
public:
    explicit MonotonicConcurrentStatisticsMap(std::size_t initial_capacity = 8);

    /// @brief Find value by key view. No copying of key arguments.
    utils::OptionalRef<const T> Find(const StatisticsKeyView& key) const;

    /// @overload
    utils::OptionalRef<T> Find(const StatisticsKeyView& key);

    /// @brief Get or create value for the given key view.
    /// @return Pair of reference to value and bool (true if inserted).
    template <typename... Args>
    std::pair<T&, bool> TryEmplace(const StatisticsKeyView& key_view, Args&&... value_args);

    /// @brief Visit all entries.
    template <typename Visitor>
    void Visit(Visitor visitor) const;

    /// @overload
    template <typename Visitor>
    void Visit(Visitor visitor);

private:
    using Entry = StatisticsMapEntry<T>;
    using Set = concurrent::impl::MonotonicConcurrentSet<Entry, StatisticsMapEntryHash<T>, StatisticsMapEntryEqual<T>>;

    Set set_;
};

template <typename T>
MonotonicConcurrentStatisticsMap<T>::MonotonicConcurrentStatisticsMap(std::size_t initial_capacity)
    : set_(initial_capacity)
{}

template <typename T>
utils::OptionalRef<T> MonotonicConcurrentStatisticsMap<T>::Find(const StatisticsKeyView& key) {
    auto ref = set_.Find(key);
    if (ref) {
        return utils::OptionalRef<T>(ref->value);
    }
    return std::nullopt;
}

template <typename T>
utils::OptionalRef<const T> MonotonicConcurrentStatisticsMap<T>::Find(const StatisticsKeyView& key) const {
    auto ref = set_.Find(key);
    if (ref) {
        return utils::OptionalRef<const T>(ref->value);
    }
    return std::nullopt;
}

template <typename T>
template <typename... Args>
std::pair<T&, bool> MonotonicConcurrentStatisticsMap<
    T>::TryEmplace(const StatisticsKeyView& key_view, Args&&... value_args) {
    auto [entry, inserted] = set_.TryEmplace(key_view, key_view, std::forward<Args>(value_args)...);
    return {entry.value, inserted};
}

template <typename T>
template <typename Visitor>
void MonotonicConcurrentStatisticsMap<T>::Visit(Visitor visitor) const {
    set_.Visit([&visitor](const Entry& entry) { visitor(entry.key, entry.value); });
}

template <typename T>
template <typename Visitor>
void MonotonicConcurrentStatisticsMap<T>::Visit(Visitor visitor) {
    set_.Visit([&visitor](Entry& entry) { visitor(entry.key, entry.value); });
}

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
