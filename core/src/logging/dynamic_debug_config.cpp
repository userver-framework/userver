#include "dynamic_debug_config.hpp"

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/level_serialization.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

bool operator==(const DynamicDebugConfig& a, const DynamicDebugConfig& b) {
  return a.force_enabled == b.force_enabled &&
         a.force_disabled == b.force_disabled;
}

DynamicDebugConfig Parse(const formats::json::Value& value,
                         formats::parse::To<DynamicDebugConfig>) {
  DynamicDebugConfig result;

  result.force_enabled =
      value["force-enabled-level"]
          .As<std::unordered_map<std::string, logging::Level>>({});
  result.force_disabled =
      value["force-disabled-level"]
          .As<std::unordered_map<std::string, logging::Level>>({});

  for (auto location : value["force-enabled"].As<std::vector<std::string>>()) {
    result.force_enabled[std::move(location)] = logging::Level::kTrace;
  }
  for (auto location : value["force-disabled"].As<std::vector<std::string>>()) {
    // You can not disable WARNING and ERROR logs using this method
    result.force_disabled[std::move(location)] = logging::Level::kInfo;
  }

  return result;
}

}  // namespace logging

USERVER_NAMESPACE_END
