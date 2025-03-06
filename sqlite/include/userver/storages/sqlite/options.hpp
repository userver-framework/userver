#pragma once

/// @file userver/storages/sqlite/options.hpp
/// @brief Options

#include <optional>
#include <string>

#include <userver/components/component_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::settings {

// TODO: Is an isolation level switch necessary? By default Serializable, but
// using pragma (PRAGMA read_uncommitted = TRUE) we can make it Read Uncommitted

struct TransactionOptions {
  enum Mode { kDeferred = 0, kImmediate = 1, kExclusive = 2 };
  Mode mode = kDeferred;

  constexpr TransactionOptions() = default;
  constexpr explicit TransactionOptions(Mode m) : mode{m} {}

  bool IsReadOnly() const { return mode & kImmediate; }

  static constexpr TransactionOptions Deferred() {
    return TransactionOptions{kDeferred};
  }
};

constexpr inline bool operator==(const TransactionOptions& lhs,
                                 const TransactionOptions& rhs) {
  return lhs.mode == rhs.mode;
}

/// Default size limit for prepared statements cache
inline constexpr std::size_t kDefaultMaxPreparedCacheSize = 200;

struct ConnectionSettings {
  enum PreparedStatementOptions {
    kCachePreparedStatements,
    kNoPreparedStatements,
  };

  /// Cache prepared statements or not
  PreparedStatementOptions prepared_statements = kCachePreparedStatements;

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

struct CommandControl {
  enum OperationType {
    kReadOnly,
    kReadWrite,
  };

  OperationType operation_type;

  static constexpr CommandControl ReadWrite() {
    return CommandControl{OperationType::kReadWrite};
  }
  static constexpr CommandControl ReadOnly() {
    return CommandControl{OperationType::kReadOnly};
  }
  static constexpr CommandControl GetDefault() { return ReadWrite(); }
};

using OptionalCommandControl = std::optional<CommandControl>;

inline constexpr bool kDefaultCreateFile = true;
inline constexpr bool kDefaultSharedCashe = false;
inline constexpr bool kDefaultWALMode = true;

struct SQLiteSettings {
  enum class ReadMode { kReadOnly = 0, kReadWrite = 1 };
  ReadMode read_mode = ReadMode::kReadWrite;
  bool create_file = kDefaultCreateFile;
  bool shared_cashe = kDefaultSharedCashe;
  bool wal_mode = kDefaultWALMode;
  std::string db_name;
  ConnectionSettings conn_settings;
  PoolSettings pool_settings;
};

}  // namespace storages::sqlite::settings

USERVER_NAMESPACE_END
