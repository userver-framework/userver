#pragma once

/// @file formats/parse/to.hpp
/// @brief Helper for parsers.

namespace formats::parse {

/// An ADL helper that allows searching for `Parse` functions in namespace
/// `formats::parse` additionally to the namespace of `T`.
template <class T>
struct To {};

}  // namespace formats::parse
