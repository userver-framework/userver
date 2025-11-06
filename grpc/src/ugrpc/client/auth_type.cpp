#include <userver/ugrpc/client/auth_type.hpp>

#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

AuthType Parse(const yaml_config::YamlConfig& value, formats::parse::To<AuthType>) {
    constexpr utils::TrivialBiMap kMap([](auto selector) {
        return selector().Case(AuthType::kInsecure, "insecure").Case(AuthType::kSsl, "ssl");
    });

    return utils::ParseFromValueString(value, kMap);
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
