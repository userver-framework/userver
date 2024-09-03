#pragma once

#include "int.hpp"

#include <userver/chaotic/object.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet kns__Int_PropertiesNames =
    [](auto selector) {
      return selector().template Type<std::string_view>().Case("foo");
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
