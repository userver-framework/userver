#pragma once

#include "int_enum.hpp"

#include <fmt/format.h>
#include <cstdint>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/object.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialBiMap kns__Enum__Foo_Mapping =
    [](auto selector) {
      return selector()
          .template Type<ns::Enum::Foo, int>()
          .Case(ns::Enum::Foo::kInner, 0)
          .Case(ns::Enum::Foo::kLeft, 1)
          .Case(ns::Enum::Foo::kRight, 2)
          .Case(ns::Enum::Foo::kOuter, 3)
          .Case(ns::Enum::Foo::k5, 5);
    };

static constexpr USERVER_NAMESPACE::utils::TrivialSet
    kns__Enum_PropertiesNames = [](auto selector) {
      return selector().template Type<std::string_view>().Case("foo");
    };

template <typename Value>
ns::Enum::Foo Parse(Value val,
                    USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo>) {
  const auto value = val.template As<std::int32_t>();
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
