#include <userver/utest/utest.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/server/middlewares/cors.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kAnyOriginWithCredentials = R"(
allowed-origins:
  - '*'
allow-credentials: true
)";

constexpr std::string_view kAnyOriginWithoutCredentials = R"(
allowed-origins:
  - '*'
allow-credentials: false
)";

constexpr std::string_view kConcreteOriginWithCredentials = R"(
allowed-origins:
  - https://mysite.example
allow-credentials: true
)";

std::unique_ptr<server::middlewares::Cors> MakeCors(std::string_view static_config) {
    const auto config = yaml_config::YamlConfig(formats::yaml::FromString(std::string{static_config}), {})
                            .As<server::middlewares::Cors::Config>();
    return std::make_unique<server::middlewares::Cors>(config);
}

}  // namespace

TEST(CorsMiddleware, AnyOriginWithCredentialsIsRejected) {
    UEXPECT_THROW_MSG(MakeCors(kAnyOriginWithCredentials), std::runtime_error, "allow-credentials");
}

TEST(CorsMiddleware, AnyOriginWithoutCredentialsIsAccepted) {
    UEXPECT_NO_THROW(MakeCors(kAnyOriginWithoutCredentials));
}

TEST(CorsMiddleware, ConcreteOriginWithCredentialsIsAccepted) {
    UEXPECT_NO_THROW(MakeCors(kConcreteOriginWithCredentials));
}

USERVER_NAMESPACE_END
