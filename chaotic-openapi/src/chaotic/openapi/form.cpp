#include <userver/chaotic/openapi/form.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

std::string PrimitiveToString(const std::string& value) { return value; }

std::string PrimitiveToString(std::int64_t value) { return std::to_string(value); }

std::string PrimitiveToString(std::int32_t value) { return std::to_string(value); }

std::string PrimitiveToString(double value) { return std::to_string(value); }

std::string PrimitiveToString(bool value) { return value ? "true" : "false"; }

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
