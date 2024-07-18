#pragma once

#include "int.hpp"

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet kns__Int_PropertiesNames =
    [](auto selector) {
      return selector().template Type<std::string>().Case("foo");
    };

template <typename Value>
ns::Int Parse(Value value, USERVER_NAMESPACE::formats::parse::To<ns::Int>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  ns::Int res;

  res.foo =
      value["foo"]
          .template As<
              std::optional<USERVER_NAMESPACE::chaotic::Primitive<int>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
      value, kns__Int_PropertiesNames);

  return res;
}

}  // namespace ns
