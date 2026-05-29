#pragma once

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/array.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/ref.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/validators_pattern.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

#include "docs.hpp"

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__Circle_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("kind").Case("radius");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Circle Parse(Value value, USERVER_NAMESPACE::formats::parse::To<Circle>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  Circle res;

  res.kind = value["kind"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();
  res.radius = value["radius"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<double>>>();

  res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
      Parse(std::move(value), USERVER_NAMESPACE::formats::parse::To<USERVER_NAMESPACE::formats::json::Value>()),
      k__ns__Circle_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__Object_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("foo").Case("bar");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Object Parse(Value value, USERVER_NAMESPACE::formats::parse::To<Object>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  Object res;

  res.foo = value["foo"].template As<USERVER_NAMESPACE::chaotic::Primitive<int>>();
  res.bar = value["bar"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__Object_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__ObjectCpp_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("some-hyphenated-key");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
ObjectCpp Parse(Value value, USERVER_NAMESPACE::formats::parse::To<ObjectCpp>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ObjectCpp res;

  res.some_hyphenated_key =
      value["some-hyphenated-key"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__ObjectCpp_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__Rectangle_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("kind").Case("width").Case("height");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Rectangle Parse(Value value, USERVER_NAMESPACE::formats::parse::To<Rectangle>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  Rectangle res;

  res.kind = value["kind"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();
  res.width = value["width"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<double>>>();
  res.height = value["height"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<double>>>();

  res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
      Parse(std::move(value), USERVER_NAMESPACE::formats::parse::To<USERVER_NAMESPACE::formats::json::Value>()),
      k__ns__Rectangle_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialBiMap k__ns__Status_Mapping = [](auto selector) {
  return selector()
      .template Type<Status, std::string_view>()
      .Case(Status::kPending, "pending")
      .Case(Status::kRunning, "running")
      .Case(Status::kDone, "done");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
Status Parse(Value val, USERVER_NAMESPACE::formats::parse::To<Status>) {
  const auto value = val.template As<std::string>();
  const auto result = k__ns__Status_Mapping.TryFindBySecond(value);
  if (result.has_value()) {
    return *result;
  }
  USERVER_NAMESPACE::chaotic::ThrowForValue(fmt::format("Invalid enum value ({}) for type ::ns::Status", value), val);
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__TreeNode_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("data").Case("left").Case("right");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
TreeNode Parse(Value value, USERVER_NAMESPACE::formats::parse::To<TreeNode>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  TreeNode res;

  res.data = value["data"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();
  res.left =
      value["left"]
          .template As<
              std::optional<USERVER_NAMESPACE::chaotic::Ref<USERVER_NAMESPACE::chaotic::Primitive<::ns::TreeNode>>>>();
  res.right =
      value["right"]
          .template As<
              std::optional<USERVER_NAMESPACE::chaotic::Ref<USERVER_NAMESPACE::chaotic::Primitive<::ns::TreeNode>>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__TreeNode_PropertiesNames);

  return res;
}

}  // namespace ns

