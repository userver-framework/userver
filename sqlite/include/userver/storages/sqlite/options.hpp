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

struct ConnectionSettings {};

struct CommandControl {};

using OptionalCommandControl = std::optional<CommandControl>;

struct SQLiteSettings {
  enum class ReadMode { kReadOnly = 0, kReadWrite = 1 };
  ReadMode read_mode = ReadMode::kReadWrite;
  bool create_file = true;
  std::string db_name;
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
