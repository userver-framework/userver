#pragma once

/// @file userver/formats/parse/to.hpp
/// @brief Helper for parsers.
///
/// @ingroup userver_formats_parse

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

/// @ingroup userver_formats_parse
///
/// An ADL helper that allows searching for `Parse` functions in namespace
/// `formats::parse` additionally to the namespace of `T`.
///
/// @see @ref md_en_userver_formats
template <class T>
struct To {};

}  // namespace formats::parse

USERVER_NAMESPACE_END
