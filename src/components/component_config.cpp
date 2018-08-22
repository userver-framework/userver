#include <yandex/taxi/userver/components/component_config.hpp>

namespace components {

ComponentConfig::ComponentConfig(Json::Value json, std::string full_path,
                                 json_config::VariableMapPtr config_vars_ptr)
    : json_config::JsonConfig(std::move(json), std::move(full_path),
                              std::move(config_vars_ptr)) {
  name_ = ParseString("name");
}

const std::string& ComponentConfig::Name() const { return name_; }

ComponentConfig ComponentConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  return {json, full_path, config_vars_ptr};
}

}  // namespace components
