#pragma once

/// @file userver/utils/boost_uuid7s.hpp
/// @brief @copybrief utils::generators::GenerateBoostUuidV7()

#include <boost/uuid/uuid.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::generators {

/// @brief Generates UUIDv7
///
/// Uses 22-bit counter to ensure UUID's monotonicity in generated batches (or
/// for UUID's generated for the same timestamp). See RFC for detailed UUID
/// format info:
/// https://datatracker.ietf.org/doc/html/draft-ietf-uuidrev-rfc4122bis#name-uuid-version-7
boost::uuids::uuid GenerateBoostUuidV7();

}  // namespace utils::generators

USERVER_NAMESPACE_END
