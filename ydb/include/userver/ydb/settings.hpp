#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <ydb-cpp-sdk/client/table/query_stats/stats.h>

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

enum class TransactionMode { kSerializableRW, kOnlineRO, kStaleRO, kSnapshotRO, kSnapshotRW };

struct OperationSettings final {
    std::optional<std::uint32_t> retries{std::nullopt};

    // https://docs.yandex-team.ru/ydb-tech/best_practices/timeouts#operational
    std::chrono::milliseconds client_timeout_ms{0};
    std::optional<TransactionMode> tx_mode{std::nullopt};
    std::chrono::milliseconds get_session_timeout_ms{0};

    std::string trace_id{};
};

struct QuerySettings final {
    // deprecated, Query Client doesn't have KeepInQueryCache, it caches automatically
    std::optional<bool> keep_in_query_cache{std::nullopt};

    std::optional<NYdb::NTable::ECollectQueryStatsMode> collect_query_stats{std::nullopt};
};

struct RequestSettings final {
    std::chrono::milliseconds timeout_ms{0};

    std::string trace_id{};
};

using ExecuteSettings = RequestSettings;
using CommitSettings = RequestSettings;
using RollbackSettings = RequestSettings;

struct RetryTxSettings final {
    TransactionMode tx_mode{TransactionMode::kSerializableRW};
    std::chrono::milliseconds timeout_ms{0};
    std::uint32_t retries{10};
    bool is_idempotent{false};

    CommitSettings commit_settings;
    RollbackSettings rollback_settings;

    std::string trace_id{};
};

}  // namespace ydb

namespace formats::parse {

ydb::OperationSettings Parse(const yaml_config::YamlConfig& config, To<ydb::OperationSettings>);

}  // namespace formats::parse

USERVER_NAMESPACE_END
