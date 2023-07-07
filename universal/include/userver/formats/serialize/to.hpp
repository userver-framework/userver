#pragma once

/// @file userver/formats/serialize/to.hpp
/// @brief Helper for serializers.
/// @ingroup userver_formats_serialize

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

/// @ingroup userver_formats_serialize
///
/// An ADL helper that allows searching for `Serialize` functions in namespace
/// `formats::serialize` additionally to the namespace of `T`.
template <class T>
struct To {};

}  // namespace formats::serialize

USERVER_NAMESPACE_END
