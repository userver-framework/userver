#pragma once

/// @file userver/storages/sqlite/options.hpp
/// @brief SQLite options

#include <string>

#include <userver/components/component_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::settings {

/// @brief SQLite transaction options.
struct TransactionOptions {
    /// @brief Locking mode for the transaction.
    ///
    /// Determines how database locks are acquired during the transaction.
    /// @see https://www.sqlite.org/lang_transaction.html
    enum LockingMode {
        kDeferred,   ///< Locks are acquired as needed (default behavior).
        kImmediate,  ///< Acquires a reserved lock immediately.
        kExclusive   ///< Acquires an exclusive lock immediately.
    };

    /// @brief Isolation level for the transaction.
    ///
    /// SQLite supports limited isolation levels. Note that full support for
    /// READ UNCOMMITTED requires shared-cache mode.
    /// @see https://www.sqlite.org/isolation.html
    enum class IsolationLevel {
        kSerializable,    ///< Default isolation level; ensures full isolation.
        kReadUncommitted  ///< Allows reading uncommitted changes; requires shared-cache mode.
    };

    IsolationLevel isolation_level = IsolationLevel::kSerializable;
    LockingMode mode = LockingMode::kDeferred;

    constexpr TransactionOptions() = default;
    constexpr explicit TransactionOptions(IsolationLevel lvl) : isolation_level{lvl} {}
    constexpr TransactionOptions(IsolationLevel lvl, LockingMode m) : isolation_level{lvl}, mode{m} {}
    constexpr explicit TransactionOptions(LockingMode m) : mode{m} {}

    /// @brief Creates a TransactionOptions instance with deferred locking mode.
    static constexpr TransactionOptions Deferred() { return TransactionOptions{kDeferred}; }
};

constexpr inline bool operator==(const TransactionOptions& lhs, const TransactionOptions& rhs) {
    return lhs.mode == rhs.mode;
}

std::string IsolationLevelToString(const TransactionOptions::IsolationLevel& lvl);

/// @brief Default maximum size for the prepared statements cache.
inline constexpr std::size_t kDefaultMaxPreparedCacheSize = 200;

/// @brief Default setting for caching prepared statements.
inline constexpr bool kDefaultPrepareStatement = true;

/// @brief SQLite connection settings.
///
/// Configures behavior related to prepared statements and their caching.
struct ConnectionSettings {
    /// @brief Options for handling prepared statements.
    enum PreparedStatementOptions {
        kCachePreparedStatements,  ///< Enable caching of prepared statements.
        kNoPreparedStatements,     ///< Disable caching of prepared statements.
    };

    /// Cache prepared statements or not
    PreparedStatementOptions prepared_statements =
        kDefaultPrepareStatement ? kCachePreparedStatements : kNoPreparedStatements;

    /// Maximum number of prepared statements to cache
    std::size_t max_prepared_cache_size = kDefaultMaxPreparedCacheSize;

    static ConnectionSettings Create(const components::ComponentConfig& config);
};

/// @brief Default initial size for the connection pool
inline constexpr std::size_t kDefaultInitialPoolSize = 5;

/// @brief Default maximum size for the connection pool
inline constexpr std::size_t kDefaultMaxPoolSize = 10;

/// @brief SQLite connection pool settings.
///
/// Configures the behavior of the connection pool, including its size.
struct PoolSettings final {
    /// @brief Number of connections created initially.
    std::size_t initial_pool_size{kDefaultInitialPoolSize};

    /// @brief Maximum number of connections in the pool.
    std::size_t max_pool_size{kDefaultMaxPoolSize};

    static PoolSettings Create(const components::ComponentConfig& config);
};

inline constexpr bool kDefaultCreateFile = true;
inline constexpr bool kDefaultIsReadOnly = false;
inline constexpr bool kDefaultSharedCache = false;
inline constexpr bool kDefaultReadUncommitted = false;
inline constexpr bool kDefaultForeignKeys = true;
inline constexpr std::string_view kDefaultJournalMode = "wal";
inline constexpr std::string_view kDefaultSynchronous = "normal";
inline constexpr std::string_view kDefaultTempStore = "memory";
inline constexpr int kDefaultBusyTimeout = 5000;
inline constexpr int kDefaultCacheSize = -2000;
inline constexpr int kDefaultJournalSizeLimit = 67108864;
inline constexpr int kDefaultMmapSize = 134217728;
inline constexpr int kDefaultPageSize = 4096;

/// @brief Comprehensive SQLite settings.
///
/// Aggregates various settings for configuring SQLite behavior.
struct SQLiteSettings {
    /// @brief Read mode for the database.
    enum class ReadMode {
        kReadOnly,  ///< Open the database in read-only mode
        kReadWrite  ///< Open the database in read-write mode
    };

    /// @brief Journal mode options.
    enum class JournalMode {
        kDelete,    ///< Delete the journal file after each transaction.
        kTruncate,  ///< Truncate the journal file to zero length.
        kPersist,   ///< Keep the journal file but zero its header.
        kMemory,    ///< Store the journal in memory.
        kWal,       ///< Use write-ahead logging.
        kOff        ///< Disable the journal.
    };

    /// @brief Synchronous setting options.
    enum Synchronous {
        kExtra,   ///< Extra synchronization.
        kFull,    ///< Full synchronization.
        kNormal,  ///< Normal synchronization.
        kOff      ///< No synchronization.
    };

    /// @brief Temporary store options.
    enum TempStore {
        kMemory,  ///< Store temporary tables in memory.
        kFile     ///< Store temporary tables in a file.
    };

    ReadMode read_mode = !kDefaultIsReadOnly ? ReadMode::kReadWrite : ReadMode::kReadOnly;
    bool create_file = kDefaultCreateFile;
    bool shared_cache = kDefaultSharedCache;
    bool read_uncommitted = kDefaultReadUncommitted;
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
