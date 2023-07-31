#pragma once

#include <type_traits>

#include <fmt/format.h>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/server/http/http_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

template <typename Value,
          typename = std::enable_if_t<formats::common::kIsFormatValue<Value>>>
HttpStatus Parse(const Value& value, formats::parse::To<HttpStatus>) {
  using IntType = std::underlying_type_t<HttpStatus>;
  constexpr IntType kMinCode = 100;
  constexpr IntType kMaxCode = 599;

  const auto integer = value.template As<IntType>();
  if (integer < kMinCode || integer > kMaxCode) {
    throw typename Value::Exception(
        fmt::format("HttpStatus value {} out of [{}..{}] range at '{}'",
                    integer, kMinCode, kMaxCode, value.GetPath()));
  }

  return HttpStatus{integer};
}

}  // namespace server::http

USERVER_NAMESPACE_END
