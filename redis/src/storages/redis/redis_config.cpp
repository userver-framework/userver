#include <storages/redis/redis_config.hpp>

namespace redis {

redis::CommandControl::Strategy ParseJson(const formats::json::Value& elem,
                                          ::redis::CommandControl::Strategy*) {
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

::redis::CommandControl ParseJson(const formats::json::Value& elem,
                                  ::redis::CommandControl*) {
  ::redis::CommandControl response;

  for (auto it = elem.begin(); it != elem.end(); ++it) {
    const auto& name = it.GetName();
    if (name == "timeout_all_ms") {
      response.timeout_all = std::chrono::milliseconds(it->As<unsigned>());
    } else if (name == "timeout_single_ms") {
      response.timeout_single = std::chrono::milliseconds(it->As<unsigned>());
    } else if (name == "max_retries") {
      response.max_retries = it->As<unsigned>();
    } else if (name == "strategy") {
      response.strategy = it->As<::redis::CommandControl::Strategy>();
    } else if (name == "best_dc_count") {
      response.best_dc_count = it->As<unsigned>();
    } else if (name == "max_ping_latency_ms") {
      response.max_ping_latency = std::chrono::milliseconds(it->As<unsigned>());
    } else {
      LOG_WARNING() << "unknown key for CommandControl map: " << name;
    }
  }
  if ((response.best_dc_count > 1) &&
      (response.strategy !=
       ::redis::CommandControl::Strategy::kNearestServerPing)) {
    LOG_WARNING() << "CommandControl.best_dc_count = " << response.best_dc_count
                  << ", but is ignored for the current strategy ("
                  << (size_t)response.strategy << ")";
  }
  return response;
}

}  // namespace redis
