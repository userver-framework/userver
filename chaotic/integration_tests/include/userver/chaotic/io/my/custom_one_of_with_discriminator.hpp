#pragma once

#include <string>
#include <variant>

#include <userver/chaotic/convert.hpp>
#include <userver/utils/assert.hpp>

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
    std::variant<U, V>&& value,
    USERVER_NAMESPACE::chaotic::convert::To<CustomOneOfWithDiscriminator>) {
  U* u = std::get_if<U>(&value);
  if (u) return {u->field1};
  V* v = std::get_if<V>(&value);
  if (v) return {v->field2};
  UASSERT(false);
  return {};
}

std::variant<ns::CustomStruct1, ns::CustomStruct2> Convert(
    const CustomOneOfWithDiscriminator& value,
    USERVER_NAMESPACE::chaotic::convert::To<
        std::variant<ns::CustomStruct1, ns::CustomStruct2>>);

}  // namespace my
