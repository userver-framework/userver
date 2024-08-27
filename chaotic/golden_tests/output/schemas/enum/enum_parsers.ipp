#pragma once

#include "enum.hpp"

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialBiMap kns__Enum__Foo_Mapping =
    [](auto selector) {
      return selector()
          .template Type<ns::Enum::Foo, std::string_view>()
          .Case(ns::Enum::Foo::kOne, "one")
          .Case(ns::Enum::Foo::kTwo, "two")
          .Case(ns::Enum::Foo::kThree, "three");
    };

static constexpr USERVER_NAMESPACE::utils::TrivialSet
    kns__Enum_PropertiesNames = [](auto selector) {
      return selector().template Type<std::string_view>().Case("foo");
    };

template <typename Value>
ns::Enum::Foo Parse(Value val,
                    USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo>) {
  const auto value = val.template As<std::string>();
  const auto result = kns__Enum__Foo_Mapping.TryFindBySecond(value);
  if (result.has_value()) {
    return *result;
  }
  USERVER_NAMESPACE::chaotic::ThrowForValue(
      fmt::format("Invalid enum value ({}) for type ns::Enum::Foo", value),
      val);
}

template <typename Value>
ns::Enum Parse(Value value, USERVER_NAMESPACE::formats::parse::To<ns::Enum>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ns::Enum res;

  res.foo = value["foo"]
                .template As<std::optional<
                    USERVER_NAMESPACE::chaotic::Primitive<ns::Enum::Foo>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
      value, kns__Enum_PropertiesNames);

  return res;
}

}  // namespace ns
