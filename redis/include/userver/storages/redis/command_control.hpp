#pragma once

/// @file
/// @brief @copybrief storages::redis::CommandControl

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include <userver/compiler/impl/three_way_comparison.hpp>
#include <userver/storages/redis/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {
struct RedisControl;
}  // namespace testsuite

namespace storages::redis {

inline constexpr std::chrono::milliseconds kDefaultTimeoutSingle{500};
inline constexpr std::chrono::milliseconds kDefaultTimeoutAll{2000};
inline constexpr std::size_t kDefaultMaxRetries{4};

/// Opaque Id of Redis server instance / any server instance.
class ServerId {
public:
    /// Default: any server
    constexpr ServerId() = default;

    bool IsAny() const { return id_ == -1; }

#ifdef USERVER_IMPL_HAS_THREE_WAY_COMPARISON
    auto operator<=>(const ServerId&) const = default;
#else
    bool operator==(const ServerId& other) const { return other.id_ == id_; }
    bool operator!=(const ServerId& other) const { return !(other == *this); }

    bool operator<(const ServerId& other) const { return id_ < other.id_; }
#endif

    static ServerId Generate() {
        ServerId sid;
        sid.id_ = next_id_++;
        return sid;
    }

    static ServerId Invalid() { return invalid_; }

    int64_t GetId() const { return id_; }

    void SetDescription(std::string description) const;
    void RemoveDescription() const;
    std::string GetDescription() const;

private:
    static std::atomic<std::int64_t> next_id_;
    static ServerId invalid_;

    std::int64_t id_{-1};
};

struct ServerIdHasher {
    std::size_t operator()(ServerId server_id) const noexcept { return std::hash<std::size_t>{}(server_id.GetId()); }
};

struct RetryNilFromMaster {};

/// Can be used as an additional parameter in some commands to force retries to
/// master if slave returned a nil reply.
inline constexpr RetryNilFromMaster kRetryNilFromMaster{};

/// Redis command execution options
struct CommandControl {
    enum class Strategy {
        /// Same as kEveryDc
        kDefault,

        /// Send ~1/N requests to an instance with ping N ms
        kEveryDc,

        /// Send requests to Redis instances located in local DC (by Conductor info)
        kLocalDcConductor,

        /// Send requests to 'best_dc_count' Redis instances with the min ping
        kNearestServerPing,
    };

    /// Timeout for a single attempt to execute command
    std::optional<std::chrono::milliseconds> timeout_single;

    /// Command execution timeout, including retries
    std::optional<std::chrono::milliseconds> timeout_all;

    /// The maximum number of retries while executing command
    std::optional<std::size_t> max_retries;

    /// Server instance selection strategy
    std::optional<Strategy> strategy{};

    /// How many nearest DCs to use
    std::optional<std::size_t> best_dc_count{};

    /// Force execution on master node
    std::optional<bool> force_request_to_master{};

    /// Consider ping to nodes in instance selection (true if not specified).
    /// Setting to false makes the load on the database evenly distributed, but may increase timings
    std::optional<bool> consider_ping{};

    /// Server latency limit
    std::optional<std::chrono::milliseconds> max_ping_latency{};

    /// Allow execution of readonly commands on master node along with replica
    /// nodes to facilitate load distribution
    std::optional<bool> allow_reads_from_master{};

    /// Controls if the command execution accounted in statistics
    std::optional<bool> account_in_statistics{};

    /// If set, force execution on specific shard
    std::optional<std::size_t> force_shard_idx{};

    /// Split execution of multi-key commands (i.e., MGET) to multiple requests
    std::optional<std::size_t> chunk_size{};

    /// If set, the user wants a specific Redis instance to handle the command.
    /// Sentinel may not redirect the command to other instances. strategy is
    /// ignored.
    std::optional<ServerId> force_server_id{};

    /// If set, command retries are directed to the master instance
    bool force_retries_to_master_on_nil_reply{false};

    /// Need to be set to if you do manual retries and want retry budget to work.
    /// If set value other than 0 then request treated as retry.
    /// 0 - original request, 1 - first retry, 2 - second and so on
    size_t retry_counter{0};

    constexpr CommandControl() = default;
    constexpr CommandControl(
        const std::optional<std::chrono::milliseconds>& timeout_single,
        const std::optional<std::chrono::milliseconds>& timeout_all,
        const std::optional<size_t>& max_retries
    ) noexcept
        : timeout_single(timeout_single), timeout_all(timeout_all), max_retries(max_retries) {}

#ifdef USERVER_IMPL_HAS_THREE_WAY_COMPARISON
    auto operator<=>(const CommandControl&) const = default;
#endif

    CommandControl MergeWith(const CommandControl& b) const;
    CommandControl MergeWith(const testsuite::RedisControl&) const;
    CommandControl MergeWith(RetryNilFromMaster) const;

    std::string ToString() const;
};

/// Returns CommandControl::Strategy from string
CommandControl::Strategy StrategyFromString(std::string_view s);

/// Returns string representation of CommandControl::Strategy
std::string_view StrategyToString(CommandControl::Strategy s);

}  // namespace storages::redis

USERVER_NAMESPACE_END
