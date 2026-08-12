#pragma once

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename Value>
class Error final : public Value::ParseException {
    using Value::ParseException::ParseException;
};

template <typename Value>
[[noreturn]] inline void ThrowForValue(std::string_view str, const Value& value) {
    throw Error<Value>(fmt::format("Error at path '{}': {}", value.GetPath(), str));
}

template <typename Value>
[[noreturn]] inline void ThrowForPath(std::string_view str, std::string_view path) {
    throw Error<Value>(fmt::format("Error at path '{}': {}", path, str));
}

}  // namespace chaotic

USERVER_NAMESPACE_END
