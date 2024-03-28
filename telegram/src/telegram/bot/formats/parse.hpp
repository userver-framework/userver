#pragma once

#include <memory>

#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

template <class T, class Value>
std::unique_ptr<T> Parse(const Value& value,
                         formats::parse::To<std::unique_ptr<T>>) {
  if (value.IsMissing() || value.IsNull()) {
    return nullptr;
  }
  return std::make_unique<T>(value.template As<T>());
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
