#pragma once

#include <type_traits>

#include <fmt/format.h>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/http/status_code.hpp>

USERVER_NAMESPACE_BEGIN

namespace http {

template <typename Value,
          typename = std::enable_if_t<formats::common::kIsFormatValue<Value>>>
StatusCode Parse(const Value& value, formats::parse::To<StatusCode>) {
  using IntType = std::underlying_type_t<StatusCode>;
  constexpr IntType kMinCode = 100;
  constexpr IntType kMaxCode = 599;

  const auto integer = value.template As<IntType>();
  if (integer < kMinCode || integer > kMaxCode) {
    throw typename Value::Exception(
        fmt::format("StatusCode value {} out of [{}..{}] range at '{}'",
                    integer, kMinCode, kMaxCode, value.GetPath()));
  }

  return StatusCode{integer};
}

}  // namespace http

USERVER_NAMESPACE_END
