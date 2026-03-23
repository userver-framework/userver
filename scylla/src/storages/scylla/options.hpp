#pragma once

/// @file userver/storages/scylla/options.hpp
/// @brief Query options

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

USERVER_NAMESPACE_BEGIN

/// Table operation options
namespace storages::scylla::options {

/// @brief Consistency level for reads and writes
/// @see https://docs.scylladb.com/stable/cql/consistency.html
class Consistency {
public:
    enum Level {
        /// Enough for a response from a single replica
        kOne,
        /// Enough for a response from two replicas
        kTwo,
        /// Enough for a response from three replicas
        kThree,
        /// Response from a majority of replicas
        kQuorum,
        /// Response from all replicas
        kAll,
        /// Response from a majority of replicas in the local datacenter
        kLocalQuorum,
        /// Response from a quorum in each datacenter
        kEachQuorum,
        /// Response from a single replica in the local datacenter
        kLocalOne,
        /// Write is guaranteed to have been written to at least one node (writes only)
        kAny,
    };

    explicit Consistency(Level level) : level_(level) {}

    Level GetLevel() const { return level_; }

private:
    Level level_;
};

/// @brief Serial consistency level for lightweight transactions (LWT)
/// @note Only meaningful for conditional writes (IF NOT EXISTS / IF <condition>)
/// @see https://docs.scylladb.com/stable/using-scylla/lwt.html
class SerialConsistency {
public:
    enum Level {
        /// Linearizable consistency across all datacenters
        kSerial,
        /// Linearizable consistency within the local datacenter only
        kLocalSerial,
    };

    explicit SerialConsistency(Level level) : level_(level) {}

    Level GetLevel() const { return level_; }

private:
    Level level_;
};

/// @brief Per-query request timeout
/// @warning Overrides the default client-side timeout. The server-side timeout
/// is separate and configured on the Scylla node.
class Timeout {
public:
    explicit Timeout(std::chrono::milliseconds value) : value_(value) {}

    const std::chrono::milliseconds& Value() const { return value_; }

private:
    std::chrono::milliseconds value_;
};

/// @brief Client-side timestamp for the mutation
/// @note Scylla uses microsecond-resolution timestamps for conflict resolution
/// (last-write-wins). If not set, the driver generates one automatically.
/// @see https://docs.scylladb.com/stable/cql/types.html
class Timestamp {
public:
    explicit Timestamp(std::chrono::microseconds value) : value_(value) {}

    /// Convenience: construct from a system_clock time_point
    explicit Timestamp(std::chrono::system_clock::time_point tp)
        : value_(std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch())) {}

    std::chrono::microseconds Value() const { return value_; }
    int64_t Microseconds() const { return value_.count(); }

private:
    std::chrono::microseconds value_;
};

/// @brief Time-to-live for inserted/updated data
/// @note After the TTL expires, Scylla removes the data during compaction.
/// A TTL of 0 means the data does not expire.
/// @see https://docs.scylladb.com/stable/cql/time-to-live.html
class Ttl {
public:
    explicit Ttl(std::chrono::seconds value) : value_(value) {}

    std::chrono::seconds Value() const { return value_; }
    int32_t Seconds() const { return static_cast<int32_t>(value_.count()); }

private:
    std::chrono::seconds value_;
};

/// @brief Enables request tracing on the server side
/// @note The trace can be retrieved from `system_traces.sessions` and
/// `system_traces.events` tables after execution.
/// @see https://docs.scylladb.com/stable/using-scylla/tracing.html
class Tracing {
public:
    explicit Tracing(bool enabled = true) : enabled_(enabled) {}

    bool IsEnabled() const { return enabled_; }

private:
    bool enabled_;
};

/// @brief Limits the number of rows returned per page for SELECT queries
/// @note Does not limit the total result set, only how many rows the server
/// returns in a single response. Use with PagingState for iteration.
class PageSize {
public:
    explicit PageSize(int32_t value) : value_(value) {}

    int32_t Value() const { return value_; }

private:
    int32_t value_;
};

/// @brief Paging state token for resuming a paged query
/// @note Obtain from a previous query's result metadata. Passing an invalid
/// or expired token is a server error.
class PagingState {
public:
    explicit PagingState(std::string token) : token_(std::move(token)) {}

    const std::string& Token() const { return token_; }

private:
    std::string token_;
};

/// @brief Specifies the target node or datacenter for query routing
/// @note Scylla drivers are token-aware by default, but this can override
/// routing for special cases.
class RoutingKey {
public:
    explicit RoutingKey(std::string value) : value_(std::move(value)) {}

    const std::string& Value() const { return value_; }

private:
    std::string value_;
};

/// @brief Retry policy override for a single query
/// @see https://docs.scylladb.com/stable/using-scylla/driver-retry-policy.html
class RetryPolicy {
public:
    enum Policy {
        /// Use the session-level default
        kDefault,
        /// Retry on the next host in the query plan
        kFallthrough,
    };

    explicit RetryPolicy(Policy policy) : policy_(policy) {}

    Policy GetPolicy() const { return policy_; }

private:
    Policy policy_;
};

/// @brief Speculative execution policy for latency-sensitive reads
/// @note Sends redundant requests to other replicas after a delay, uses
/// whichever responds first.
class SpeculativeExecution {
public:
    /// @param max_attempts Maximum extra requests to send (not counting the original)
    /// @param delay Delay before sending each speculative request
    SpeculativeExecution(int32_t max_attempts, std::chrono::milliseconds delay)
        : max_attempts_(max_attempts), delay_(delay) {}

    int32_t MaxAttempts() const { return max_attempts_; }
    std::chrono::milliseconds Delay() const { return delay_; }

private:
    int32_t max_attempts_;
    std::chrono::milliseconds delay_;
};

/// Applies IF NOT EXISTS to an INSERT, making it a lightweight transaction
/// @note Uses Paxos consensus, significantly slower than a regular insert.
class IfNotExists {};

/// Applies IF EXISTS to an UPDATE or DELETE, making it a lightweight transaction
/// @note Uses Paxos consensus, significantly slower than regular mutations.
class IfExists {};

/// Enables ALLOW FILTERING on a SELECT query
/// @warning Full-cluster scans are expensive. Avoid in production unless
/// you understand the performance implications.
/// @see https://docs.scylladb.com/stable/using-scylla/allow-filtering.html
class AllowFiltering {};

/// Uses UNLOGGED batch instead of the default LOGGED batch
/// @note Logged batches provide atomicity guarantees across partitions;
/// unlogged batches do not, but are cheaper.
class Unlogged {};

/// @brief Disables exception throw on server errors, the error should be
/// checked manually in the result
class SuppressServerExceptions {};

/// @brief Sets a comment/label for the query visible in tracing and slow query logs
class Comment {
public:
    explicit Comment(std::string value) : value_(std::move(value)) {}

    const std::string& Value() const { return value_; }

private:
    std::string value_;
};

}  // namespace storages::scylla::options

USERVER_NAMESPACE_END