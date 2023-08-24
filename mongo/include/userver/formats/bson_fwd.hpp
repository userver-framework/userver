#pragma once

/// @file userver/formats/bson_fwd.hpp
/// @brief Forward declarations of formats::bson types, formats::parse::To and
/// formats::serialize::To.

#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {

class Oid;
class Binary;
class Decimal128;
class Document;
class Timestamp;
class Value;
class ValueBuilder;

}  // namespace formats::bson

USERVER_NAMESPACE_END
