#pragma once

/// @file userver/formats/json_fwd.hpp
/// @brief Forward declarations of formats::json types, formats::parse::To and
/// formats::serialize::To.

#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

class StringBuilder;

// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
class Value;

// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
class ValueBuilder;

}  // namespace formats::json

USERVER_NAMESPACE_END
