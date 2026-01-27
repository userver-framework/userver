#pragma once

#include <stdexcept>

#include <userver/formats/json/value.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename Value>
class Error final : public Value::Exception {
    using Value::Exception::Exception;
};

template <typename Value>
[[noreturn]] inline void ThrowForValue(std::string_view str, Value value) {
    throw Error<Value>(fmt::format("Error at path '{}': {}", value.GetPath(), str));
}

}  // namespace chaotic

USERVER_NAMESPACE_END
