#include <userver/cache/update_type.hpp>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace {

constexpr utils::TrivialBiMap kUpdateTypeMap([](auto selector) {
  return selector()
      .Case(UpdateType::kFull, "full")
      .Case(UpdateType::kIncremental, "incremental");
});

constexpr utils::TrivialBiMap kAllowedUpdateTypesMap([](auto selector) {
  return selector()
      .Case(AllowedUpdateTypes::kFullAndIncremental, "full-and-incremental")
      .Case(AllowedUpdateTypes::kOnlyFull, "only-full")
      .Case(AllowedUpdateTypes::kOnlyIncremental, "only-incremental");
});

}  // namespace

UpdateType Parse(const formats::json::Value& value,
                 formats::parse::To<UpdateType>) {
  return utils::ParseFromValueString(value, kUpdateTypeMap);
}

std::string_view ToString(UpdateType update_type) {
  return utils::impl::EnumToStringView(update_type, kUpdateTypeMap);
}

AllowedUpdateTypes Parse(const yaml_config::YamlConfig& value,
                         formats::parse::To<AllowedUpdateTypes>) {
  return utils::ParseFromValueString(value, kAllowedUpdateTypesMap);
}

std::string_view ToString(AllowedUpdateTypes allowed_update_types) {
  return utils::impl::EnumToStringView(allowed_update_types,
                                       kAllowedUpdateTypesMap);
}

}  // namespace cache

USERVER_NAMESPACE_END
