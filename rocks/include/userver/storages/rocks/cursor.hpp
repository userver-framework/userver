#pragma once

/// @file userver/storages/rocks/cursor.hpp
/// @brief @copybrief storages::rocks::Cursor

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <rocksdb/db.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/rocks/key_value.hpp>

namespace rocksdb {
class Iterator;
class Snapshot;
}  // namespace rocksdb

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

class Client;
class Snapshot;

/// @brief Streaming cursor for iterating over RocksDB entries in key order.
///
/// Created by Client::Scan() or Snapshot::Scan(). Fetches data in batches;
/// call FetchBatch() repeatedly until it returns an empty vector.
///
/// The cursor holds an internal snapshot that fixes the read horizon at the
/// moment Scan() was called — concurrent writes are not visible.
///
/// @warning Holding a Cursor for a long time prevents RocksDB from compacting
/// away superseded key versions, causing unbounded space amplification.
/// Release cursors as soon as they are no longer needed.
class Cursor final {
public:
    Cursor(const Cursor&) = delete;
    Cursor& operator=(const Cursor&) = delete;
    Cursor(Cursor&&) noexcept;
    Cursor& operator=(Cursor&&) noexcept;
    ~Cursor();

    /// Returns the next batch of key-value pairs, in key order.
    /// Returns an empty vector when the scan is complete.
    /// @throws storages::rocks::RequestFailedException on RocksDB I/O error.
    std::vector<KeyValue> FetchBatch(std::size_t batch_size = 100);

private:
    friend class Client;
    friend class Snapshot;
    Cursor(
        std::shared_ptr<rocksdb::DB> db,
        engine::TaskProcessor& tp,
        const rocksdb::Snapshot* snap,
        std::string prefix
    );

    std::shared_ptr<rocksdb::DB> db_;
    engine::TaskProcessor* tp_;
    const rocksdb::Snapshot* snap_;
    // it_ declared after snap_ so it is destroyed first
    // (iterator must not outlive the snapshot).
    std::unique_ptr<rocksdb::Iterator> it_;
    std::string prefix_;
    bool done_{false};
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
