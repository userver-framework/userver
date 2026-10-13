#include <userver/utest/utest.hpp>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/middlewares/cors.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <components/component_list_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kInvalidCredentialsMessage = "'allow-credentials: true' must not be combined";

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

constexpr std::string_view kAnyOriginWithDefaultCredentials = R"(
allowed-origins:
  - '*'
)";

constexpr std::string_view kConcreteOriginWithCredentials = R"(
allowed-origins:
  - https://mysite.example
allow-credentials: true
)";

constexpr std::string_view kCorsServiceConfig = R"(
components_manager:
  components:
    server:
      listener:
        task_processor: main-task-processor
    default-server-middleware-pipeline-builder:
      append:
        - cors-middleware
    handler-ping:
      path: /ping
      method: GET,OPTIONS
)";

std::unique_ptr<server::middlewares::Cors> MakeCors(std::string_view static_config) {
    const auto config =
        yaml_config::YamlConfig(formats::yaml::FromString(std::string{static_config}), {})
            .As<server::middlewares::Cors::Config>();
    return std::make_unique<server::middlewares::Cors>(config);
}

void RunCorsService(std::string_view global_config, std::string_view handler_config) {
    const auto temp_directory = fs::blocking::TempDirectory::Create();
    formats::yaml::ValueBuilder config{
        formats::yaml::FromString(tests::MergeYaml(tests::kMinimalStaticConfig, kCorsServiceConfig))
    };
    auto components_config = config["components_manager"]["components"];
    components_config["server"]["listener"]["unix-socket"] = temp_directory.GetPath() + "/http.sock";
    components_config["cors-middleware"] = formats::yaml::FromString(std::string{global_config});
    auto handler_middlewares = components_config["handler-ping"]["middlewares"];
    handler_middlewares["cors-middleware"] = formats::yaml::FromString(std::string{handler_config});

    const auto component_list =
        components::MinimalServerComponentList()
            .Append<server::middlewares::CorsFactory>()
            .Append<server::handlers::Ping>();
    components::RunOnce(components::InMemoryConfig{formats::yaml::ToString(config.ExtractValue())}, component_list);
}

}  // namespace

TEST(CorsMiddleware, AnyOriginWithCredentialsIsRejected) {
    UEXPECT_THROW_MSG(MakeCors(kAnyOriginWithCredentials), std::runtime_error, kInvalidCredentialsMessage);
}

TEST(CorsMiddleware, ManuallyConstructedAnyOriginWithCredentialsIsRejected) {
    server::middlewares::Cors::Config config;
    config.allowed_origins = {"*"};
    config.allow_credentials = true;

    UEXPECT_THROW_MSG(server::middlewares::Cors{config}, std::runtime_error, kInvalidCredentialsMessage);
}

TEST(CorsMiddleware, MixedOriginsWithCredentialsIsRejected) {
    constexpr std::array<std::string_view, 3> kMixedOrigins = {
        "['*', 'https://first.example', 'https://second.example']",
        "['https://first.example', '*', 'https://second.example']",
        "['https://first.example', 'https://second.example', '*']",
    };
    for (const auto origins : kMixedOrigins) {
        SCOPED_TRACE(origins);
        const auto config = "allowed-origins: " + std::string{origins} + "\nallow-credentials: true\n";
        UEXPECT_THROW_MSG(MakeCors(config), std::runtime_error, kInvalidCredentialsMessage);
    }
}

TEST(CorsMiddleware, AnyOriginWithoutCredentialsIsAccepted) {
    UEXPECT_NO_THROW(MakeCors(kAnyOriginWithoutCredentials));
}

TEST(CorsMiddleware, AnyOriginWithDefaultCredentialsIsAccepted) {
    UEXPECT_NO_THROW(MakeCors(kAnyOriginWithDefaultCredentials));
}

TEST(CorsMiddleware, ConcreteOriginWithCredentialsIsAccepted) {
    UEXPECT_NO_THROW(MakeCors(kConcreteOriginWithCredentials));
}

using CorsFactoryConfig = ComponentList;

TEST_F(CorsFactoryConfig, EnablingCredentialsWithInheritedAnyOriginIsRejected) {
    UEXPECT_THROW_MSG(
        RunCorsService(kAnyOriginWithoutCredentials, "allow-credentials: true"),
        std::runtime_error,
        kInvalidCredentialsMessage
    );
}

TEST_F(CorsFactoryConfig, AnyOriginOverrideWithInheritedCredentialsIsRejected) {
    UEXPECT_THROW_MSG(
        RunCorsService(kConcreteOriginWithCredentials, "allowed-origins: ['*']"),
        std::runtime_error,
        kInvalidCredentialsMessage
    );
}

TEST_F(CorsFactoryConfig, ConcreteOriginOverrideWithCredentialsIsAccepted) {
    UEXPECT_NO_THROW(RunCorsService(kAnyOriginWithoutCredentials, kConcreteOriginWithCredentials));
}

TEST_F(CorsFactoryConfig, DisablingCredentialsForInheritedAnyOriginIsAccepted) {
    UEXPECT_NO_THROW(RunCorsService(kAnyOriginWithCredentials, "allow-credentials: false"));
}

TEST_F(CorsFactoryConfig, ConcreteOriginOverrideWithInheritedCredentialsIsAccepted) {
    UEXPECT_NO_THROW(RunCorsService(kAnyOriginWithCredentials, "allowed-origins: ['https://mysite.example']"));
}

USERVER_NAMESPACE_END
