#pragma once

/// @file userver/storages/sqlite/options.hpp
/// @brief Options

#include <string>

#include <userver/components/component_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::settings {

struct TransactionOptions {
    enum LockingMode { kDeferred, kImmediate, kExclusive };
    enum class IsolationLevel { kSerializable, kReadUncommitted };  // Read Uncommitted to work requires shared-cashe

    IsolationLevel isolation_level = IsolationLevel::kSerializable;
    LockingMode mode = LockingMode::kDeferred;

    constexpr TransactionOptions() = default;
    constexpr explicit TransactionOptions(IsolationLevel lvl) : isolation_level{lvl} {}
    constexpr TransactionOptions(IsolationLevel lvl, LockingMode m) : isolation_level{lvl}, mode{m} {}
    constexpr explicit TransactionOptions(LockingMode m) : mode{m} {}

    static constexpr TransactionOptions Deferred() { return TransactionOptions{kDeferred}; }
};

constexpr inline bool operator==(const TransactionOptions& lhs, const TransactionOptions& rhs) {
    return lhs.mode == rhs.mode;
}

std::string IsolationLevelToString(const TransactionOptions::IsolationLevel& lvl);

inline constexpr std::size_t kDefaultMaxPreparedCacheSize = 200;
inline constexpr bool kDefaultPrepareStatement = true;

struct ConnectionSettings {
    enum PreparedStatementOptions {
        kCachePreparedStatements,
        kNoPreparedStatements,
    };

    /// Cache prepared statements or not
    PreparedStatementOptions prepared_statements =
        kDefaultPrepareStatement ? kCachePreparedStatements : kNoPreparedStatements;

    /// Limits the size or prepared statements cache
    std::size_t max_prepared_cache_size = kDefaultMaxPreparedCacheSize;

    static ConnectionSettings Create(const components::ComponentConfig& config);
};

/// Default connection pool settings
inline constexpr std::size_t kDefaulInitialPoolSize = 5;
inline constexpr std::size_t kDefaultMaxPoolSize = 10;

struct PoolSettings final {
    std::size_t initial_pool_size{kDefaulInitialPoolSize};
    std::size_t max_pool_size{kDefaultMaxPoolSize};

    static PoolSettings Create(const components::ComponentConfig& config);
};

inline constexpr bool kDefaultCreateFile = true;
inline constexpr bool kDefaultIsReadOnly = false;
inline constexpr bool kDefaultSharedCashe = false;
inline constexpr bool kDefaultReadUncommited = false;
inline constexpr bool kDefaultForeignKeys = true;
inline constexpr std::string_view kDefaultJournalMode = "wal";
inline constexpr std::string_view kDefaultSynchronous = "normal";
inline constexpr std::string_view kDefaultTempStore = "memory";
inline constexpr int kDefaultBusyTimeout = 5000;
inline constexpr int kDefaultCacheSize = -2000;
inline constexpr int kDefaultJournalSizeLimit = 67108864;
inline constexpr int kDefaultMmapSize = 134217728;
inline constexpr int kDefaultPageSize = 4096;

struct SQLiteSettings {
    enum class ReadMode { kReadOnly, kReadWrite };
    enum class JournalMode { kDelete, kTruncate, kPersist, kMemory, kWal, kOff };
    enum Synchronous { kExtra, kFull, kNormal, kOff };
    enum TempStore { kMemory, kFile };

    ReadMode read_mode = !kDefaultIsReadOnly ? ReadMode::kReadWrite : ReadMode::kReadOnly;
    bool create_file = kDefaultCreateFile;
    bool shared_cashe = kDefaultSharedCashe;
    bool read_uncommited = kDefaultReadUncommited;
    bool foreign_keys = kDefaultForeignKeys;
    JournalMode journal_mode = JournalMode::kWal;
    int busy_timeout = kDefaultBusyTimeout;
    Synchronous synchronous = Synchronous::kNormal;
    int cache_size = kDefaultCacheSize;
    TempStore temp_store = TempStore::kMemory;
    int journal_size_limit = kDefaultJournalSizeLimit;
    int mmap_size = kDefaultMmapSize;
    int page_size = kDefaultPageSize;
    std::string db_path;
    ConnectionSettings conn_settings;
    PoolSettings pool_settings;
};

std::string JournalModeToString(const SQLiteSettings::JournalMode& mode);

}  // namespace storages::sqlite::settings

USERVER_NAMESPACE_END
