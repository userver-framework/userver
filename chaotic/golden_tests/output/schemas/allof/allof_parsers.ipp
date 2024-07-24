#pragma once

#include "allof.hpp"

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet
    kns__AllOf__Foo__P0_PropertiesNames = [](auto selector) {
      return selector().template Type<std::string_view>().Case("foo");
    };

static constexpr USERVER_NAMESPACE::utils::TrivialSet
    kns__AllOf__Foo__P1_PropertiesNames = [](auto selector) {
      return selector().template Type<std::string_view>().Case("bar");
    };

static constexpr USERVER_NAMESPACE::utils::TrivialSet
    kns__AllOf_PropertiesNames = [](auto selector) {
      return selector().template Type<std::string_view>().Case("foo");
    };

template <typename Value>
ns::AllOf::Foo__P0 Parse(
    Value value, USERVER_NAMESPACE::formats::parse::To<ns::AllOf::Foo__P0>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ns::AllOf::Foo__P0 res;

  res.foo = value["foo"]
                .template As<std::optional<
                    USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();

  res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
      value, kns__AllOf__Foo__P0_PropertiesNames);

  return res;
}

template <typename Value>
ns::AllOf::Foo__P1 Parse(
    Value value, USERVER_NAMESPACE::formats::parse::To<ns::AllOf::Foo__P1>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ns::AllOf::Foo__P1 res;

  res.bar =
      value["bar"]
          .template As<
              std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

  res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
      value, kns__AllOf__Foo__P1_PropertiesNames);

  return res;
}

template <typename Value>
ns::AllOf::Foo Parse(Value value,
                     USERVER_NAMESPACE::formats::parse::To<ns::AllOf::Foo>) {
  return ns::AllOf::Foo(value.template As<ns::AllOf::Foo__P0>(),
                        value.template As<ns::AllOf::Foo__P1>());
}

template <typename Value>
ns::AllOf Parse(Value value, USERVER_NAMESPACE::formats::parse::To<ns::AllOf>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ns::AllOf res;

  res.foo = value["foo"]
                .template As<std::optional<
                    USERVER_NAMESPACE::chaotic::Primitive<ns::AllOf::Foo>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
      value, kns__AllOf_PropertiesNames);

  return res;
}

}  // namespace ns
