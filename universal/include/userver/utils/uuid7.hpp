#pragma once

/// @file utils/uuid7.hpp
/// @brief @copybrief utils::generators::GenerateUuidV7
/// @ingroup userver_universal

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils::generators {

/// @brief Generate a UUIDv7 string
std::string GenerateUuidV7();

}  // namespace utils::generators

USERVER_NAMESPACE_END
