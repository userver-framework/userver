#include "dynamic_debug_config.hpp"

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

bool operator==(const DynamicDebugConfig& a, const DynamicDebugConfig& b) {
  return a.force_enabled == b.force_enabled &&
         a.force_disabled == b.force_disabled;
}

DynamicDebugConfig Parse(const formats::json::Value& value,
                         formats::parse::To<DynamicDebugConfig>) {
  return DynamicDebugConfig{
      value["force-enabled"].As<std::vector<std::string>>(),
      value["force-disabled"].As<std::vector<std::string>>()};
}

}  // namespace logging

USERVER_NAMESPACE_END
