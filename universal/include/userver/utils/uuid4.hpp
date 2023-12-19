#pragma once

/// @file utils/uuid4.hpp
/// @brief @copybrief utils::generators::GenerateUuid
/// @ingroup userver_universal

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils::generators {

/// @brief Generate a UUID string
std::string GenerateUuid();

}  // namespace utils::generators

USERVER_NAMESPACE_END
