#include <userver/components/static_config_validator.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace components {

ValidationMode Parse(const yaml_config::YamlConfig& value, formats::parse::To<ValidationMode>) {
    if (value["validate_all_components"].As<bool>()) {
        return ValidationMode::kAll;
    } else {
        return ValidationMode::kOnlyTurnedOn;
    }
}

namespace impl {

[[noreturn]] void WrapInvalidStaticConfigSchemaException(const std::exception& ex) {
    throw std::runtime_error(fmt::format("Invalid static config schema: {}", ex.what()));
}

}  // namespace impl

}  // namespace components

USERVER_NAMESPACE_END
