#pragma once

#include <string>
#include <variant>

#include <userver/chaotic/convert.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

// From chaotic
namespace ns {
struct CustomStruct1;
struct CustomStruct2;
}  // namespace ns

namespace my {

struct CustomOneOfWithDiscriminator {
  CustomOneOfWithDiscriminator() = default;

  CustomOneOfWithDiscriminator(int x) : val(x) {}
  CustomOneOfWithDiscriminator(std::string x) : val(x) {}

  std::variant<int, std::string> val;
};

inline bool operator==(const CustomOneOfWithDiscriminator& lhs,
                       const CustomOneOfWithDiscriminator& rhs) {
  return lhs.val == rhs.val;
}

template <typename U, typename V>
CustomOneOfWithDiscriminator Convert(
    const std::variant<U, V>& value,
    USERVER_NAMESPACE::chaotic::convert::To<CustomOneOfWithDiscriminator>) {
  return USERVER_NAMESPACE::utils::Visit(
      value,
      [](const U& value) { return CustomOneOfWithDiscriminator{value.field1}; },
      [](const V& value) {
        return CustomOneOfWithDiscriminator{value.field2};
      });
}

std::variant<ns::CustomStruct1, ns::CustomStruct2> Convert(
    const CustomOneOfWithDiscriminator& value,
    USERVER_NAMESPACE::chaotic::convert::To<
        std::variant<ns::CustomStruct1, ns::CustomStruct2>>);

}  // namespace my
