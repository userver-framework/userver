#include <userver/components/static_config_validator.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ValidationMode Parse(const yaml_config::YamlConfig& value,
                     formats::parse::To<ValidationMode>) {
  if (value["validate_all_components"].As<bool>()) {
    return ValidationMode::kAll;
  } else {
    return ValidationMode::kOnlyTurnedOn;
  }
}

}  // namespace components

USERVER_NAMESPACE_END
