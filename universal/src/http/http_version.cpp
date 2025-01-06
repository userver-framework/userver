#include <userver/http/http_version.hpp>

#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace http {

HttpVersion Parse(const yaml_config::YamlConfig& value, formats::parse::To<HttpVersion>) {
    static constexpr utils::TrivialBiMap kMap([](auto selector) {
        return selector().Case(HttpVersion::k2, "2").Case(HttpVersion::k11, "1.1");
    });
    return utils::ParseFromValueString(value, kMap);
}

}  // namespace http

USERVER_NAMESPACE_END
