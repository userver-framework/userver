#pragma once

/// @file userver/storages/sqlite/options.hpp
/// @brief Options

#include <optional>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

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
};

struct CommandControl {};

using OptionalCommandControl = std::optional<CommandControl>;

struct SQLiteSettings {
  enum class ReadMode { kReadOnly = 0, kReadWrite = 1 };
  ReadMode read_mode = ReadMode::kReadWrite;
  bool create_file = true;
  std::string db_name;
  ConnectionSettings conn_settings;
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
