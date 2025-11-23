#include <userver/clients/http/cancellation_policy.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

CancellationPolicy Parse(yaml_config::YamlConfig value, formats::parse::To<CancellationPolicy>) {
    auto str = value.As<std::string>();
    if (str == "cancel") {
        return CancellationPolicy::kCancel;
    }
    if (str == "ignore") {
        return CancellationPolicy::kIgnore;
    }
    throw std::runtime_error("Invalid CancellationPolicy value: " + str);
}

}  // namespace clients::http

USERVER_NAMESPACE_END
