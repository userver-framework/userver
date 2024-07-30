#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename RawType, typename... Validators>
struct Primitive final {
  const RawType& value;
};

template <typename Value, typename RawType, typename... Validators>
RawType Parse(const Value& value,
              formats::parse::To<Primitive<RawType, Validators...>>) {
  auto result = value.template As<RawType>();
  chaotic::Validate<Validators...>(result, value);
  return result;
}

template <typename Value, typename RawType, typename... Validators>
Value Serialize(const Primitive<RawType, Validators...>& ps,
                formats::serialize::To<Value>) {
  return typename Value::Builder{ps.value}.ExtractValue();
}

}  // namespace chaotic

USERVER_NAMESPACE_END
