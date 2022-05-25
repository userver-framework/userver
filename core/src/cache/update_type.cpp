#include <userver/cache/update_type.hpp>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

AllowedUpdateTypes Parse(const yaml_config::YamlConfig& value,
                         formats::parse::To<AllowedUpdateTypes>) {
  const auto str = value.As<std::string>();
  if (str == "full-and-incremental") {
    return AllowedUpdateTypes::kFullAndIncremental;
  } else if (str == "only-full") {
    return AllowedUpdateTypes::kOnlyFull;
  } else if (str == "only-incremental") {
    return AllowedUpdateTypes::kOnlyIncremental;
  }
  throw yaml_config::YamlConfig::Exception(fmt::format(
      "Invalid AllowedUpdateTypes value '{}' at '{}'", str, value.GetPath()));
}

UpdateType Parse(const formats::json::Value& value,
                 formats::parse::To<UpdateType>) {
  const auto str = value.As<std::string>();
  if (str == "full") {
    return UpdateType::kFull;
  } else if (str == "incremental") {
    return UpdateType::kIncremental;
  }

  throw formats::json::ParseException(fmt::format(
      "Invalid UpdateType value '{}' at '{}'", str, value.GetPath()));
}

}  // namespace cache

USERVER_NAMESPACE_END
