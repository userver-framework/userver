#pragma once

/// @file userver/storages/rocks/map.hpp
/// @brief @copybrief storages::rocks::Map

#include <optional>
#include <string>
#include <string_view>

#include <userver/formats/parse/to.hpp>
#include <userver/storages/rocks/raw_map.hpp>
#include <userver/storages/rocks/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

/// @brief Typed read-only map view of a RocksDB database at a fixed point in time.
///
/// Wraps RawMap and applies ADL-based serialisation:
/// - Keys are serialised via @c ToString(key) → std::string.
/// - Values are deserialised via @c Parse(raw, formats::parse::To<Value>{}).
///
/// @warning Each call to operator[]() dispatches to a blocking task processor
/// and suspends the calling coroutine.
///
/// @warning Compaction is blocked while a Map is alive.
/// Destroy it as soon as the read phase is complete.
template <typename Key, typename Value>
class Map final {
public:
    /// Takes ownership of @p snapshot and fixes the read horizon at that point.
    static Map FromSnapshot(Snapshot snapshot) { return Map{RawMap::FromSnapshot(std::move(snapshot))}; }

    Map(const Map&) = delete;
    Map& operator=(const Map&) = delete;
    Map(Map&&) noexcept = default;
    Map& operator=(Map&&) noexcept = default;

    /// Serialises @p key with ToString(), looks up in the snapshot, and
    /// deserialises the result with Parse(raw, To<Value>{}).
    /// Returns nullopt if the key is absent.
    std::optional<Value> operator[](const Key& key) const {
        const auto raw = raw_[ToString(key)];
        if (!raw) {
            return std::nullopt;
        }
        return Parse(std::string_view{*raw}, formats::parse::To<Value>{});
    }

private:
    explicit Map(RawMap raw)
        : raw_(std::move(raw))
    {}

    RawMap raw_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
