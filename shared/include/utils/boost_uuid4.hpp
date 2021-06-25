#pragma once

/// @file utils/boost_uuid4.hpp
/// @brief @copybrief utils::generators::GenerateBoostUuid()

#include <boost/uuid/uuid.hpp>

/// Generators
namespace utils::generators {

/// Generates UUID
boost::uuids::uuid GenerateBoostUuid();

}  // namespace utils::generators
