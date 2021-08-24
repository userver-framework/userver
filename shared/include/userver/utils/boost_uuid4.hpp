#pragma once

/// @file userver/utils/boost_uuid4.hpp
/// @brief @copybrief utils::generators::GenerateBoostUuid()

#include <boost/uuid/uuid.hpp>

#include <fmt/core.h>

/// Generators
namespace utils::generators {

/// Generates UUID
boost::uuids::uuid GenerateBoostUuid();

namespace impl {
std::string ToString(const boost::uuids::uuid&);
}  // namespace impl

}  // namespace utils::generators

template <>
struct fmt::formatter<boost::uuids::uuid> {
  static auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const boost::uuids::uuid& uuid, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "{}",
                          utils::generators::impl::ToString(uuid));
  }
};
