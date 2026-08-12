#pragma once

/// @file userver/utils/boost_uuid5.hpp
/// @brief @copybrief utils::generators::GenerateBoostUuidV5()

#include <string_view>

#include <boost/uuid/uuid.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::generators {

/// RFC 9562 name-based UUID namespace IDs (same as boost::uuids::ns::*, see boost_uuid5.cpp).
namespace ns {

extern const boost::uuids::uuid kDns;
extern const boost::uuids::uuid kUrl;
extern const boost::uuids::uuid kOid;
extern const boost::uuids::uuid kX500dn;

}  // namespace ns

/// Generates UUIDv5 (name-based, SHA-1) for the given namespace and name
///
/// @see https://datatracker.ietf.org/doc/html/rfc9562#section-5.5
boost::uuids::uuid GenerateBoostUuidV5(const boost::uuids::uuid& namespace_uuid, std::string_view name);

}  // namespace utils::generators

USERVER_NAMESPACE_END
