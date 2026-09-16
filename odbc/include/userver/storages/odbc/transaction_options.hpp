#pragma once

/// @file userver/storages/odbc/transaction_options.hpp
/// @brief ODBC transaction options

#include <cstdint>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

/// Portable ODBC transaction isolation levels.
enum class IsolationLevel : std::uint8_t {
    kReadUncommitted,
    kReadCommitted,
    kRepeatableRead,
    kSerializable,
};

/// ODBC transaction access-mode hint.
///
/// @warning `kReadOnly` requests `SQL_MODE_READ_ONLY` from the ODBC driver, but
/// ODBC defines this as an intent/optimization hint. It does not guarantee that
/// the database rejects write statements.
enum class AccessMode : std::uint8_t {
    kReadOnly,
    kReadWrite,
};

/// Options for starting an ODBC transaction.
///
/// Empty optionals preserve the physical connection's current driver defaults;
/// the driver does not silently force READ COMMITTED or READ WRITE.
struct TransactionOptions final {
    std::optional<IsolationLevel> isolation_level{};
    std::optional<AccessMode> access_mode{};

    // Explicit keeps the legacy Cluster::Begin(flags, {}) call unambiguous: an
    // empty braced argument continues to mean OptionalCommandControl.
    constexpr explicit TransactionOptions() = default;

    constexpr explicit TransactionOptions(IsolationLevel isolation)
        : isolation_level{isolation}
    {}

    constexpr explicit TransactionOptions(AccessMode mode)
        : access_mode{mode}
    {}

    constexpr TransactionOptions(IsolationLevel isolation, AccessMode mode)
        : isolation_level{isolation},
          access_mode{mode}
    {}
};

constexpr bool operator==(const TransactionOptions& lhs, const TransactionOptions& rhs) noexcept {
    return lhs.isolation_level == rhs.isolation_level && lhs.access_mode == rhs.access_mode;
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
