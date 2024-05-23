#pragma once

#include <memory>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

template <class ValueBuilder, class Value>
void SetIfNotNull(ValueBuilder& builder,
                  const std::string& key,
                  const std::optional<Value>& value) {
  if (value) {
    builder[key] = value.value();
  }
}

template <class ValueBuilder, class Value>
void SetIfNotNull(ValueBuilder& builder,
                  const std::string& key,
                  const std::unique_ptr<Value>& value) {
  if (value) {
    builder[key] = *value;
  }
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
