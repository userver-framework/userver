#pragma once

#include <stdexcept>

#include <fmt/format.h>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

class Error final : public formats::json::Exception {
  using Exception::Exception;
};

template <typename Value>
[[noreturn]] inline void ThrowForValue(std::string_view str, Value value) {
  throw Error(fmt::format("Error at path '{}': {}", value.GetPath(), str));
}

}  // namespace chaotic

USERVER_NAMESPACE_END
