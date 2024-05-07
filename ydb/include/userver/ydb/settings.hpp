#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

#include <ydb-cpp-sdk/client/table/query_stats/stats.h>

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

enum class TransactionMode { kSerializableRW, kOnlineRO, kStaleRO };

struct OperationSettings final {
  std::uint32_t retries{0};

  // https://docs.yandex-team.ru/ydb-tech/best_practices/timeouts#operational
  std::chrono::milliseconds operation_timeout_ms{0};
  std::chrono::milliseconds cancel_after_ms{0};
  std::chrono::milliseconds client_timeout_ms{0};
  std::optional<TransactionMode> tx_mode{std::nullopt};
  std::chrono::milliseconds get_session_timeout_ms{0};

  // @cond
  std::string_view trace_id{};
  // @endcond
};

struct QuerySettings final {
  std::optional<bool> keep_in_query_cache{std::nullopt};
  std::optional<NYdb::NTable::ECollectQueryStatsMode> collect_query_stats{
      std::nullopt};
};

}  // namespace ydb

namespace formats::parse {

ydb::OperationSettings Parse(const yaml_config::YamlConfig& config,
                             To<ydb::OperationSettings>);

}  // namespace formats::parse

USERVER_NAMESPACE_END
