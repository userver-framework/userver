#pragma once

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/to.hpp>

#include <userver/chaotic/convert.hpp>
#include <userver/chaotic/convert/to.hpp>
#include <userver/chaotic/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename RawType, typename UserType>
struct WithType final {
  const UserType& value;
};

template <typename Value, typename RawType, typename UserType>
UserType Parse(Value value, formats::parse::To<WithType<RawType, UserType>>) {
  auto result = value.template As<RawType>();
  try {
    return Convert(std::move(result), convert::To<UserType>{});
  } catch (const std::exception& e) {
    chaotic::ThrowForValue(e.what(), value);
  }
}

template <typename Value, typename RawType, typename UserType>
Value Serialize(const WithType<RawType, UserType>& ps,
                formats::serialize::To<Value>) {
  return typename Value::Builder{
      RawType{Convert(ps.value,
                      convert::To<std::decay_t<decltype(RawType::value)>>())}}
      .ExtractValue();
}

}  // namespace chaotic

USERVER_NAMESPACE_END
