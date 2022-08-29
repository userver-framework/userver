#pragma once

/// @file userver/utils/boost_uuid4.hpp
/// @brief @copybrief utils::generators::GenerateBoostUuid()

#include <boost/uuid/uuid.hpp>

#include <fmt/core.h>
#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Generators
namespace generators {

/// Generates UUID
boost::uuids::uuid GenerateBoostUuid();

}  // namespace generators

/// Parse string into boost::uuids::uuid
boost::uuids::uuid BoostUuidFromString(std::string const& str);

/// Serialize boost::uuids::uuid to string
std::string ToString(const boost::uuids::uuid&);

}  // namespace utils

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<boost::uuids::uuid> {
  static auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const boost::uuids::uuid& uuid,
              FormatContext& ctx) USERVER_FMT_CONST {
    return fmt::format_to(ctx.out(), "{}",
                          USERVER_NAMESPACE::utils::ToString(uuid));
  }
};
