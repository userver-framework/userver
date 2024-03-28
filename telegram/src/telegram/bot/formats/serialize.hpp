#pragma once

#include <memory>

#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

template <class T, class Value>
Value Serialize(const std::unique_ptr<T>& value,
                formats::serialize::To<Value>) {
  if (!value) return {};

  return typename Value::Builder(*value).ExtractValue();
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
