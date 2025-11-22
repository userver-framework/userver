#include <userver/engine/deadlock_detector_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

DeadlockDetector Parse(const yaml_config::YamlConfig& value, formats::parse::To<DeadlockDetector>) {
    auto str = value.As<std::string>();
    if (str == "off") return DeadlockDetector::kOff;
    if (str == "on") return DeadlockDetector::kOff;
    if (str == "detect-only") return DeadlockDetector::kDetectOnly;
    throw std::runtime_error("Unknown value: " + str);
}

}  // namespace engine

USERVER_NAMESPACE_END
