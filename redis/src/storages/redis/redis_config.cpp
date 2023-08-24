#include <unordered_map>
#include <userver/storages/redis/redis_config.hpp>

#include <userver/logging/log.hpp>
#include <userver/storages/redis/impl/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

CommandControl::Strategy Parse(
    const formats::json::Value& elem,
    formats::parse::To<USERVER_NAMESPACE::redis::CommandControl::Strategy>) {
  auto strategy = elem.As<std::string>();
  try {
    return StrategyFromString(strategy);
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to parse strategy (e.what()=" << e.what()
                << "), falling back to EveryDC";
    return CommandControl::Strategy::kEveryDc;
  }
}

USERVER_NAMESPACE::redis::CommandControl Parse(
    const formats::json::Value& elem,
    formats::parse::To<USERVER_NAMESPACE::redis::CommandControl>) {
  USERVER_NAMESPACE::redis::CommandControl response;

  for (auto it = elem.begin(); it != elem.end(); ++it) {
    const auto& name = it.GetName();
    if (name == "timeout_all_ms") {
      response.timeout_all = std::chrono::milliseconds(it->As<int64_t>());
      if (response.timeout_all.count() < 0) {
        throw ParseConfigException(
            "Invalid timeout_all in redis CommandControl");
      }
    } else if (name == "timeout_single_ms") {
      response.timeout_single = std::chrono::milliseconds(it->As<int64_t>());
      if (response.timeout_single.count() < 0) {
        throw ParseConfigException(
            "Invalid timeout_single in redis CommandControl");
      }
    } else if (name == "max_retries") {
      response.max_retries = it->As<size_t>();
    } else if (name == "strategy") {
      response.strategy =
          it->As<USERVER_NAMESPACE::redis::CommandControl::Strategy>();
    } else if (name == "best_dc_count") {
      response.best_dc_count = it->As<size_t>();
    } else if (name == "max_ping_latency_ms") {
      response.max_ping_latency = std::chrono::milliseconds(it->As<int64_t>());
      if (response.max_ping_latency.count() < 0) {
        throw ParseConfigException(
            "Invalid max_ping_latency in redis CommandControl");
      }
    } else if (name == "allow_reads_from_master") {
      response.allow_reads_from_master = it->As<bool>();
    } else {
      LOG_WARNING() << "unknown key for CommandControl map: " << name;
    }
  }
  if ((response.best_dc_count > 1) &&
      (response.strategy != USERVER_NAMESPACE::redis::CommandControl::Strategy::
                                kNearestServerPing)) {
    LOG_WARNING() << "CommandControl.best_dc_count = " << response.best_dc_count
                  << ", but is ignored for the current strategy ("
                  << static_cast<size_t>(response.strategy) << ")";
  }
  return response;
}

WaitConnectedMode Parse(const formats::json::Value& elem,
                        formats::parse::To<WaitConnectedMode> to) {
  return Parse(elem.As<std::string>(), to);
}

RedisWaitConnected Parse(const formats::json::Value& elem,
                         formats::parse::To<RedisWaitConnected>) {
  RedisWaitConnected result;
  result.mode = elem["mode"].As<WaitConnectedMode>();
  result.throw_on_fail = elem["throw_on_fail"].As<bool>();
  result.timeout = std::chrono::milliseconds{elem["timeout-ms"].As<int>()};
  return result;
}

USERVER_NAMESPACE::redis::CommandsBufferingSettings Parse(
    const formats::json::Value& elem,
    formats::parse::To<USERVER_NAMESPACE::redis::CommandsBufferingSettings>) {
  USERVER_NAMESPACE::redis::CommandsBufferingSettings result;
  result.buffering_enabled = elem["buffering_enabled"].As<bool>();
  result.commands_buffering_threshold =
      elem["commands_buffering_threshold"].As<size_t>(0);
  result.watch_command_timer_interval = std::chrono::microseconds(
      elem["watch_command_timer_interval_us"].As<size_t>());
  return result;
}

MetricsSettings Parse(const formats::json::Value& elem,
                      formats::parse::To<MetricsSettings>) {
  MetricsSettings result;
  result.timings_enabled =
      elem["timings-enabled"].As<bool>(result.timings_enabled);
  result.command_timings_enabled =
      elem["command-timings-enabled"].As<bool>(result.command_timings_enabled);
  result.request_sizes_enabled =
      elem["request-sizes-enabled"].As<bool>(result.request_sizes_enabled);
  result.reply_sizes_enabled =
      elem["reply-sizes-enabled"].As<bool>(result.reply_sizes_enabled);
  return result;
}

ReplicationMonitoringSettings Parse(
    const formats::json::Value& elem,
    formats::parse::To<ReplicationMonitoringSettings>) {
  ReplicationMonitoringSettings result;
  result.enable_monitoring =
      elem["enable-monitoring"].As<bool>(result.enable_monitoring);
  result.restrict_requests =
      elem["forbid-requests-to-syncing-replicas"].As<bool>(
          result.restrict_requests);
  return result;
}

PubsubMetricsSettings Parse(const formats::json::Value& elem,
                            formats::parse::To<PubsubMetricsSettings>) {
  PubsubMetricsSettings result;
  result.per_shard_stats_enabled =
      elem["per-shard-stats-enabled"].As<bool>(result.per_shard_stats_enabled);
  return result;
}

}  // namespace redis

USERVER_NAMESPACE_END
