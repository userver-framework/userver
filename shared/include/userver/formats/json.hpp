#pragma once

/// @file userver/formats/json.hpp
/// @brief Include-all header for JSON support

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/iterator.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

/// Value formats representation, parsing and serialization
namespace formats {}

/// JSON support
namespace formats::json {}

USERVER_NAMESPACE_END
