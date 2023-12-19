#include "dynamic_config.hpp"

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

namespace {

std::chrono::milliseconds ParseDefaultMaxTime(
    const formats::json::Value& value) {
  return std::chrono::milliseconds{value.As<std::uint32_t>()};
}

}  // namespace

const dynamic_config::Key<std::chrono::milliseconds> kDefaultMaxTime{
    "MONGO_DEFAULT_MAX_TIME_MS",
    ParseDefaultMaxTime,
    dynamic_config::DefaultAsJsonString{"0"},
};

const dynamic_config::Key<bool> kDeadlinePropagationEnabled{
    "MONGO_DEADLINE_PROPAGATION_ENABLED_V2", true};

const dynamic_config::Key<bool> kCongestionControlEnabled{
    "MONGO_CONGESTION_CONTROL_ENABLED", false};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
