#pragma once

/// @file userver/ydb/settings.hpp
/// @brief YDB operation, query and transaction settings

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <ydb-cpp-sdk/client/table/query_stats/stats.h>

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

enum class TransactionMode { kSerializableRW, kOnlineRO, kStaleRO, kSnapshotRO, kSnapshotRW, kImplicitTx };

/// @brief Settings for a single request.
struct OperationSettings final {
    /// Maximum number of retries for a single request.
    std::optional<std::uint32_t> retries{std::nullopt};

    /// Timeout for a single request.
    std::chrono::milliseconds client_timeout_ms{0};

    /// Transaction mode.
    std::optional<TransactionMode> tx_mode{std::nullopt};

    /// Timeout for getting a session.
    std::chrono::milliseconds get_session_timeout_ms{0};

    /// Whether a single request to YDB is idempotent.
    bool is_idempotent{false};

    /// @cond
    // For internal use only.
    std::string trace_id{};
    /// @endcond
};

/// @brief Settings for a single query execution.
struct QuerySettings final {
    /// Whether to keep the query in a server-side query cache.
    /// @deprecated Ignored. Execute uses Query API, which caches queries automatically.
    std::optional<bool> keep_in_query_cache{std::nullopt};

    /// Stats collection mode for query execution.
    std::optional<NYdb::NTable::ECollectQueryStatsMode> collect_query_stats{std::nullopt};
};

/// @brief Settings for a single request inside a TxActor.
struct RequestSettings final {
    /// Timeout for a single request.
    std::optional<std::chrono::milliseconds> client_timeout_ms{std::nullopt};
};

using ExecuteSettings = RequestSettings;
using GetSessionSettings = RequestSettings;
using CommitSettings = RequestSettings;
using RollbackSettings = RequestSettings;

/// @brief Settings for a transaction lambda.
/// @see TableClient::RetryTx.
struct RetryTxSettings final {
    /// Transaction mode.
    std::optional<TransactionMode> tx_mode{std::nullopt};

    /// Timeout for an entire transaction lambda.
    /// @note std::nullopt means unlimited.
    std::optional<std::chrono::milliseconds> timeout_ms{std::nullopt};

    /// Maximum number of retries for a transaction lambda.
    std::optional<std::uint32_t> retries{std::nullopt};

    /// Whether a transaction lambda is idempotent.
    bool is_idempotent{false};

    /// Settings for a get session request.
    GetSessionSettings get_session_settings{};

    /// Settings for a commit transaction.
    CommitSettings commit_settings{};

    /// Settings for a rollback transaction.
    RollbackSettings rollback_settings{};
};

}  // namespace ydb

namespace formats::parse {

ydb::OperationSettings Parse(const yaml_config::YamlConfig& config, To<ydb::OperationSettings>);

}  // namespace formats::parse

USERVER_NAMESPACE_END
