#include <storages/redis/redis_config.hpp>

#include <redis/exception.hpp>

namespace redis {

redis::CommandControl::Strategy Parse(
    const formats::json::Value& elem,
    formats::parse::To<::redis::CommandControl::Strategy>) {
  auto strategy = elem.As<std::string>();
  if (strategy == "every_dc") {
    return redis::CommandControl::Strategy::kEveryDc;
  } else if (strategy == "default") {
    return redis::CommandControl::Strategy::kDefault;
  } else if (strategy == "local_dc_conductor") {
    return redis::CommandControl::Strategy::kLocalDcConductor;
  } else if (strategy == "nearest_server_ping") {
    return redis::CommandControl::Strategy::kNearestServerPing;
  } else {
    LOG_ERROR() << "Unknown strategy for redis::CommandControl::Strategy: "
                << strategy << ", falling back to EveryDc";
    return redis::CommandControl::Strategy::kEveryDc;
  }
}

::redis::CommandControl Parse(const formats::json::Value& elem,
                              formats::parse::To<::redis::CommandControl>) {
  ::redis::CommandControl response;

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
      response.strategy = it->As<::redis::CommandControl::Strategy>();
    } else if (name == "best_dc_count") {
      response.best_dc_count = it->As<size_t>();
    } else if (name == "max_ping_latency_ms") {
      response.max_ping_latency = std::chrono::milliseconds(it->As<int64_t>());
      if (response.max_ping_latency.count() < 0) {
        throw ParseConfigException(
            "Invalid max_ping_latency in redis CommandControl");
      }
    } else {
      LOG_WARNING() << "unknown key for CommandControl map: " << name;
    }
  }
  if ((response.best_dc_count > 1) &&
      (response.strategy !=
       ::redis::CommandControl::Strategy::kNearestServerPing)) {
    LOG_WARNING() << "CommandControl.best_dc_count = " << response.best_dc_count
                  << ", but is ignored for the current strategy ("
                  << static_cast<size_t>(response.strategy) << ")";
  }
  return response;
}

}  // namespace redis
