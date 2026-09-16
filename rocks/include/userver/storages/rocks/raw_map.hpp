#pragma once

/// @file userver/storages/rocks/raw_map.hpp
/// @brief @copybrief storages::rocks::RawMap

#include <optional>
#include <string>
#include <string_view>

#include <userver/storages/rocks/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

/// @brief Read-only map-like view of a RocksDB database at a fixed point in time.
///
/// All reads observe the database state at the moment FromSnapshot() was called,
/// regardless of concurrent writes.
///
/// @warning Each call to operator[]() dispatches to a blocking task processor
/// and suspends the calling coroutine. It is not equivalent to a regular
/// std::map lookup.
///
/// @warning Compaction is blocked while a RawMap is alive.
/// Destroy it as soon as the read phase is complete.
class RawMap final {
public:
    /// Takes ownership of @p snapshot and fixes the read horizon at that point.
    static RawMap FromSnapshot(Snapshot snapshot);

    RawMap(const RawMap&) = delete;
    RawMap& operator=(const RawMap&) = delete;
    RawMap(RawMap&&) noexcept = default;
    RawMap& operator=(RawMap&&) noexcept = default;

    /// Returns the raw value for @p key, or nullopt if the key is absent.
    std::optional<std::string> operator[](std::string_view key) const;

private:
    explicit RawMap(Snapshot snapshot);

    Snapshot snapshot_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
