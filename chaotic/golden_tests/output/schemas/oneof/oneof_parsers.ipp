#pragma once

#include "oneof.hpp"

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet
    kns__OneOf_PropertiesNames = [](auto selector) {
      return selector().template Type<std::string_view>().Case("foo");
    };

template <typename Value>
ns::OneOf Parse(Value value, USERVER_NAMESPACE::formats::parse::To<ns::OneOf>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ns::OneOf res;

  res.foo = value["foo"]
                .template As<std::optional<USERVER_NAMESPACE::chaotic::Variant<
                    USERVER_NAMESPACE::chaotic::Primitive<int>,
                    USERVER_NAMESPACE::chaotic::Primitive<std::string>>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
      value, kns__OneOf_PropertiesNames);

  return res;
}

}  // namespace ns
