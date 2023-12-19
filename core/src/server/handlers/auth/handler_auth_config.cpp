#include <userver/server/handlers/auth/handler_auth_config.hpp>

#include <string_view>

#include <fmt/format.h>

#include <userver/formats/parse/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

namespace {

std::vector<std::string> ParseTypes(const yaml_config::YamlConfig& value) {
  constexpr std::string_view kTypes = "types";
  constexpr std::string_view kType = "type";

  auto types_opt = value[kTypes].As<std::optional<std::vector<std::string>>>();
  auto type_opt = value[kType].As<std::optional<std::string>>();

  if (types_opt && type_opt) {
    throw yaml_config::ParseException(
        fmt::format("invalid handler auth config: both fields '{}' "
                    "and '{}' are set, exactly one is expected",
                    kTypes, kType));
  }

  if (types_opt) return std::move(*types_opt);
  if (type_opt) return {std::move(*type_opt)};
  throw yaml_config::ParseException(
      fmt::format("invalid handler auth config: none of fields '{}' "
                  "and '{}' was found",
                  kTypes, kType));
}

}  // namespace

HandlerAuthConfig::HandlerAuthConfig(yaml_config::YamlConfig value)
    : yaml_config::YamlConfig(std::move(value)), types_(ParseTypes(*this)) {
  if (types_.empty()) {
    throw yaml_config::ParseException(
        "types list is empty in handler auth config");
  }
}

HandlerAuthConfig Parse(const yaml_config::YamlConfig& value,
                        formats::parse::To<HandlerAuthConfig>) {
  return HandlerAuthConfig{value};
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
