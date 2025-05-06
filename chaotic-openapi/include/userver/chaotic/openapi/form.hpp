#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

std::string PrimitiveToString(const std::string& value);

std::string PrimitiveToString(std::int64_t value);

std::string PrimitiveToString(std::int32_t value);

std::string PrimitiveToString(double value);

std::string PrimitiveToString(bool value);

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
