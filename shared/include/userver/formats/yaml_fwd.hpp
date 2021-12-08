#pragma once

/// @file userver/formats/yaml_fwd.hpp
/// @brief Forward declarations of formats::yaml types, formats::parse::To and
/// formats::serialize::To.

#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

class Value;
class ValueBuilder;

}  // namespace formats::yaml

USERVER_NAMESPACE_END
