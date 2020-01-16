#pragma once

/// @file formats/serialize/to.hpp
/// @brief Helper for serializers.

namespace formats::serialize {

/// An ADL helper that allows searching for `Serialize` functions in namespace
/// `formats::serialize` additionally to the namespace of `T`.
template <class T>
struct To {};

}  // namespace formats::serialize
