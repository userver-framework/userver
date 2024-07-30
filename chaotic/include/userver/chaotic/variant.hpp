#pragma once

#include <variant>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename... T>
auto& GetVariant(std::variant<T...>& arg) {
  return arg;
}

template <typename... T>
const auto& GetVariant(const std::variant<T...>& arg) {
  return arg;
}

template <typename T>
decltype(T().AsVariant()) GetVariant(T& arg) {
  return arg.AsVariant();
}

template <typename... T>
struct Variant final {
  const std::variant<decltype(Parse(std::declval<formats::json::Value>(),
                                    formats::parse::To<T>{}))...>& value;
};

template <typename Value, typename... T>
auto Parse(const Value& value, formats::parse::To<Variant<T...>>) {
  return value.template As<std::variant<T...>>();
}

template <typename Value, typename... T>
Value Serialize(const Variant<T...>& var, formats::serialize::To<Value>) {
  return std::visit(
      utils::Overloaded{[](const formats::common::ParseType<Value, T>& item) {
        return typename Value::Builder(T{item}).ExtractValue();
      }...},
      var.value);
}

}  // namespace chaotic

USERVER_NAMESPACE_END
