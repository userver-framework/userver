#pragma once

/// @file userver/storages/rocks/snapshot.hpp
/// @brief @copybrief storages::rocks::Snapshot

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <rocksdb/db.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/rocks/cursor.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

class Client;

/// @brief Point-in-time read-only view of a RocksDB database.
///
/// All reads observe the database state at the moment
/// Client::CreateSnapshot() was called, regardless of concurrent writes.
///
/// The snapshot is released (and compaction is unblocked) when the object is
/// destroyed.
///
/// @warning Holding a Snapshot for a long time prevents RocksDB from
/// compacting away superseded key versions, causing unbounded space
/// amplification. Release snapshots as soon as they are no longer needed.
class Snapshot final {
public:
    Snapshot(const Snapshot&) = delete;
    Snapshot& operator=(const Snapshot&) = delete;
    Snapshot(Snapshot&&) noexcept;
    Snapshot& operator=(Snapshot&&) noexcept;
    ~Snapshot();

    /// Returns the value for @p key at snapshot time, or nullopt if absent.
    std::optional<std::string> Get(std::string_view key) const;

    /// Batch point-lookup at snapshot time.
    ///
    /// Result order matches @p keys order. Duplicate keys are not
    /// deduplicated — each produces its own result entry.
    std::vector<std::optional<std::string>> GetMany(const std::vector<std::string_view>& keys) const;
    std::vector<std::optional<std::string>> GetMany(const std::vector<std::string>& keys) const;

    /// Returns a streaming cursor over all keys with the given @p prefix.
    /// If @p prefix is empty, scans all keys.
    ///
    /// The snapshot is transferred into the Cursor; this Snapshot object
    /// must not be used after this call.
    Cursor Scan(std::string_view prefix = {}) &&;

private:
    friend class Client;
    friend class Cursor;
    Snapshot(std::shared_ptr<rocksdb::DB> db, engine::TaskProcessor& tp, const rocksdb::Snapshot* snap);

    std::shared_ptr<rocksdb::DB> db_;
    engine::TaskProcessor* tp_;
    const rocksdb::Snapshot* snap_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
