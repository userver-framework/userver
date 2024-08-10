#pragma once

#include "oneofdiscriminator.hpp"

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet kns__A_PropertiesNames =
    [](auto selector) {
      return selector().template Type<std::string_view>().Case("type").Case(
          "a_prop");
    };

template <typename Value>
ns::A Parse(Value value, USERVER_NAMESPACE::formats::parse::To<ns::A>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ns::A res;

  res.type = value["type"]
                 .template As<std::optional<
                     USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();
  res.a_prop =
      value["a_prop"]
          .template As<
              std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

  res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
      value, kns__A_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet kns__B_PropertiesNames =
    [](auto selector) {
      return selector().template Type<std::string_view>().Case("type").Case(
          "b_prop");
    };

template <typename Value>
ns::B Parse(Value value, USERVER_NAMESPACE::formats::parse::To<ns::B>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ns::B res;

  res.type = value["type"]
                 .template As<std::optional<
                     USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();
  res.b_prop =
      value["b_prop"]
          .template As<
              std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

  res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
      value, kns__B_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet
    kns__OneOfDiscriminator_PropertiesNames = [](auto selector) {
      return selector().template Type<std::string_view>().Case("foo");
    };

template <typename Value>
ns::OneOfDiscriminator Parse(
    Value value,
    USERVER_NAMESPACE::formats::parse::To<ns::OneOfDiscriminator>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ns::OneOfDiscriminator res;

  res.foo =
      value["foo"]
          .template As<
              std::optional<USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<
                  &ns::OneOfDiscriminator::kFoo_Settings,
                  USERVER_NAMESPACE::chaotic::Primitive<ns::A>,
                  USERVER_NAMESPACE::chaotic::Primitive<ns::B>>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
      value, kns__OneOfDiscriminator_PropertiesNames);

  return res;
}

}  // namespace ns
